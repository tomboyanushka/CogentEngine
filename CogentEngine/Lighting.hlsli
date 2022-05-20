static const float F0_NON_METAL = 0.04f;

// Need a minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

static const float PI = 3.14159265359f;

static bool WITHOUT_CORRECT_HORIZON = false;

struct DirectionalLight
{
	float4 AmbientColor;
	float4 DiffuseColor;
	float3 Direction;
	float Intensity;
};

struct PointLight
{
	float4 Color;
	float3 Position;
	float Range;
	float Intensity;
	float3 padding;
};

struct SpotLight
{
	float4 Color;
	float3 Position;
	float Intensity;
	float3 Direction;
	float Range;
	float SpotFalloff;
	float3 padding;
};

struct SphereAreaLight
{
	float4 Color;
	float3 LightPos;
	float Radius;
	float Intensity;
	float AboveHorizon;
	float2 padding;
};

struct DiscAreaLight
{
	float4 Color;
	float3 LightPos;
	float Radius;
	float3 PlaneNormal;
	float Intensity;
};

struct RectAreaLight
{
	float4 Color;
	float3 LightPos;
	float Intensity;
	float3 LightLeft;
    float LightWidth;
	float3 LightUp;
	float LightHeight;
	float3 PlaneNormal;
	float padding;
};

float Diffuse(float3 normal, float3 dirToLight)
{
	return saturate(dot(normal, dirToLight));
}
float DiffusePBR(float3 normal, float3 dirToLight)
{
	return saturate(dot(normal, dirToLight));
}

float Attenuate(float3 lightPos, float3 worldPos, float lightRange)
{
	float dist = distance(lightPos, worldPos);

	// Ranged-based attenuation
	float att = saturate(1.0f - (dist * dist / (lightRange * lightRange)));

	// Soft falloff
	return att * att;
}
// === LIGHT TYPES FOR BASIC LIGHTING ===============================
float3 CalculatePointLight(PointLight light, float3 normal, float3 worldPos, float3 camPos, float shininess, float roughness, float3 surfaceColor)
{
	// Calc light direction
	float3 toLight = normalize(light.Position - worldPos);
	float3 toCam = normalize(camPos - worldPos);

	// Calculate the light amounts
	float atten = Attenuate(light.Position, worldPos, light.Range);
	float diff = Diffuse(normal, toLight);
	//float spec = SpecularBlinnPhong(normal, toLight, toCam, shininess) * (1.0f - roughness);

	// Combine
	return (diff * surfaceColor/* + spec*/) * atten * light.Intensity * light.Color.xyz;
}

// GGX (Trowbridge-Reitz)
//
// a - Roughness
// h - Half vector
// n - Normal
// 
// D(h, n) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float SpecDistribution(float3 n, float3 h, float roughness)
{
	// Pre-calculations
	float NdotH = saturate(dot(n, h));
	float NdotH2 = NdotH * NdotH;
	float a = roughness * roughness;
	float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

	 // ((n dot h)^2 * (a^2 - 1) + 1)
	float denomToSquare = NdotH2 * (a2 - 1) + 1;
	// Can go to zero if roughness is 0 and NdotH is 1

	// Final value
	return a2 / (PI * denomToSquare * denomToSquare);
}


// Fresnel term - Schlick approx.
// 
// v - View vector
// h - Half vector
// f0 - Value when l = n (full specular color)
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 Fresnel(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
	float VdotH = saturate(dot(v, h));

	// Final value
	return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX (based on Schlick-Beckmann)
// - k is remapped to a / 2, roughness remapped to (r+1)/2
//
// n - Normal
// v - View vector
//
// G(l,v,h)
float GeometricShadowing(float3 n, float3 v, float3 h, float roughness)
{
	// End result of remapping:
	float k = pow(roughness + 1, 2) / 8.0f;
	float NdotV = saturate(dot(n, v));

	// Final value
	return NdotV / (NdotV * (1 - k) + k);
}


// Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - part of the denominator are canceled out by numerator (see below)
//
// D() - Spec Dist - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float metalness, float3 specColor, out float3 kS)
{
	// Other vectors
	float3 h = normalize(v + l);

	// Grab various functions
	float D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, specColor); // This ranges from F0_NON_METAL to actual specColor based on metalness
	float G = GeometricShadowing(n, v, h, roughness) * GeometricShadowing(n, l, h, roughness);
	kS = F;
	// Final formula
	// Denominator dot products partially canceled by G()!
	// See page 16: http://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf
	return (D * F * G) / (4 * max(dot(n, v), dot(n, l)));
}
// Calculates diffuse color based on energy conservation
//
// diffuse - Diffuse color
// specular - Specular color (including light color)
// metalness - surface metalness amount
//
// Metals should have an albedo of (0,0,0)...mostly
// See slide 65: http://blog.selfshadow.com/publications/s2014-shading-course/hoffman/s2014_pbs_physics_math_slides.pdf
float3 DiffuseEnergyConserve(float diffuse, float3 specular, float metalness)
{
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

float3 AmbientPBR(float3 kD, float metalness, float3 diffuse, float ao, float3 specular)
{
	kD *= (1.0f - metalness);
	return (kD * diffuse + specular) * ao;
}

float3 DirLightPBR(DirectionalLight light, float3 normal, float3 worldPos, float3 camPos,
	float roughness, float metalness, float3 surfaceColor, float3 specularColor,
	float3 irradiance, float3 prefilteredColor, float2 brdf)
{
	// Get normalize direction to the light
	float3 toLight = normalize(-light.Direction);
	float3 toCam = normalize(camPos - worldPos);

	// Calculate the light amounts
	float diff = DiffusePBR(normal, toLight);
	float3 kS = float3(0.f, 0.f, 0.f);
	float ao = 1.0f;


	float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor, kS);
	float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);
	// Calculate diffuse with energy conservation
	// (Reflected light doesn't get diffused)

	float3 specular = prefilteredColor * (kS * brdf.x + brdf.y);
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
	float3 diffuse = irradiance * surfaceColor;
	float3 ambient = AmbientPBR(kD, metalness, diffuse, ao, specular);

	return (balancedDiff * surfaceColor + spec) * light.Intensity * light.DiffuseColor.rgb + ambient;
	//float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	//// Combine amount with 
	//return (balancedDiff * surfaceColor + spec) /* light.Intensity*/ * light.DiffuseColor;
}

float3 PointLightPBR(PointLight light, float3 normal, float3 worldPos, float3 camPos,
	float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{
	float3 kS = float3(0.f, 0.f, 0.f);
	float ao = 1.0f;
	// Calc light direction
	float3 toLight = normalize(light.Position - worldPos);
	float3 toCam = normalize(camPos - worldPos);

	// Calculate the light amounts
	float atten = Attenuate(light.Position, worldPos, light.Range);
	float diff = DiffusePBR(normal, toLight);
	float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor, kS);

	// Calculate diffuse with energy conservation
	// (Reflected light doesn't diffuse)
	float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	// Combine
	return (balancedDiff * surfaceColor + spec) * atten * light.Intensity * light.Color.rgb;
}

float3 SpotLightPBR(SpotLight light, float3 normal, float3 worldPos, float3 camPos,
	float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{

	float3 kS = float3(0.f, 0.f, 0.f);
	float ao = 1.0f;
	// Calc light direction
	float3 toLight = normalize(light.Position - worldPos);
	float3 toCam = normalize(camPos - worldPos);

	// Calculate the spot falloff
	float a = dot(-toLight, light.Direction);
	float b = saturate(a);

	float penumbra = pow(b, light.SpotFalloff);

	// Calculate the light amounts
	float atten = Attenuate(light.Position, worldPos, light.Range);
	float diff = DiffusePBR(normal, toLight);
	float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor, kS);

	// Calculate diffuse with energy conservation
	// (Reflected light doesn't diffuse)
	float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	// Combine
	float3 final = (balancedDiff * surfaceColor + spec) * atten * light.Intensity * light.Color.rgb;
	final = final * penumbra;
	// Combine with the point light calculation
	// Note: This could be optimized a bit
	return final;
}

// Area Lights utility functions
float3 GetDiffuseDominantDirection(float3 normal, float3 viewDir, float roughness)
{
    float NdotV = dot(normal, viewDir);
    float a = 1.02341f * roughness - 1.51174f;
    float b = -0.511705f * roughness + 0.755868f;
    float lerpFactor = saturate((NdotV * a + b) * roughness);
    return normalize(lerp(normal, viewDir, lerpFactor));
}
float3 GetSpecularDominantDirectionAreaLights(float3 N, float3 R, float roughness)
{
	// Simple linear approximation
    float lerpFactor = (1 - roughness);

    return normalize(lerp(N, R, lerpFactor));
}
float cot(float x)
{
	return cos(x) / sin(x);
}
float acot(float x)
{
	return atan(1 / x);
}

// Area Lights from moving_frostbite_to_pbr:
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
float3 AreaLightSphere(SphereAreaLight sphereLight, float3 worldPos, float3 worldNormal)
{
	float3 Lunormalized = sphereLight.LightPos - worldPos;
	float3 L = normalize(Lunormalized);
	float sqrDist = dot(Lunormalized, Lunormalized);
	float illuminance = 0.f;

	// When the light is above horizon
	if(sphereLight.AboveHorizon > 0.f)
	{
		float sqrLightRadius = sphereLight.Radius * sphereLight.Radius;
		illuminance = PI * (sqrLightRadius / (max(sqrLightRadius, sqrDist))) * saturate(dot(worldNormal, L));
	}
	else
	{
		// Tilted patch to sphere equation
		float Beta = acos(dot(worldNormal, L));
		float H = sqrt(sqrDist);
		float h = H / sphereLight.Radius;
		float x = sqrt(h * h - 1);
		float y = -x * (1 / tan(Beta));
	
		if (h * cos(Beta) > 1)
		{
			illuminance = cos(Beta) / (h * h);
		}
		else
		{
			illuminance = (1 / (PI * h * h)) *
			(cos(Beta) * acos(y) - x * sin(Beta) * sqrt(1 - y * y)) +
			(1 / PI) * atan(sin(Beta) * sqrt(1 - y * y) / x);
		}

		illuminance *= PI;
	}

	float3 result = illuminance * sphereLight.Intensity * sphereLight.Color;
	return result;
}

float3 AreaLightDisc(DiscAreaLight discLight, float3 worldPos, float3 worldNormal)
{
	float3 Lunormalized = discLight.LightPos - worldPos;
	float3 L = normalize(Lunormalized);
	float sqrDist = dot(Lunormalized, Lunormalized);
	float illuminance = 0.f;

	float3 planeNormal = normalize(discLight.PlaneNormal);

	illuminance = PI * saturate(dot(planeNormal, -L));
	illuminance *= saturate(dot(worldNormal, L));
	float div = sqrDist / (discLight.Radius * discLight.Radius);
	div += 1.0f;
	illuminance /= (sqrDist / (discLight.Radius * discLight.Radius) + 1);
	float3 result = illuminance * discLight.Intensity * discLight.Color;
	return result;
}

float RightPyramidSolidAngle(float dist, float halfWidth, float halfHeight)
{
	float a = halfWidth;
	float b = halfHeight;
	float h = dist;
	
    return 4 * asin(a * b / sqrt((a * a + h * h) * (b * b + h * h)));
}

// Most representative point: Drobot [Dro14b] proposes to approximate the illuminance integral with a single
// representative diffusepoint light weighted by the light’ s solid angle
//  The MRP is the point with the highest value when performing the diffuse lighting integral. 
// The method correctly handles the horizoncase as the light position will be moved to the MRP

float RectangleSolidAngle(float3 worldPos, float3 p0, float3 p1, float3 p2, float3 p3)
{
	float3 v0 = p0 - worldPos;
	float3 v1 = p1 - worldPos;
	float3 v2 = p2 - worldPos;
	float3 v3 = p3 - worldPos;
	
	float3 n0 = normalize(cross(v0, v1));
	float3 n1 = normalize(cross(v1, v2));
	float3 n2 = normalize(cross(v2, v3));
	float3 n3 = normalize(cross(v3, v0));
	
	float g0 = acos(dot(-n0, n1));
	float g1 = acos(dot(-n1, n2));
	float g2 = acos(dot(-n2, n3));
	float g3 = acos(dot(-n3, n0));

	return g0 + g1 + g2 + g3 - 2 * PI;
}

float3 RayPlaneIntersect(in float3 rayOrigin, in float3 rayDirection, in float3 planeOrigin, in float3 planeNormal)
{
	float distance = dot(planeNormal, planeOrigin - rayOrigin) / dot(planeNormal, rayDirection);
	return rayOrigin + rayDirection * distance;
}

float3 closestPointRect(in float3 pos, in float3 planeOrigin, in float3 left, in float3 up, in float halfWidth, in float halfHeight)
{
	float3 dir = pos - planeOrigin;
	// Project in 2D plane ( forward is the light direction away from the plane )
	// Clamp inside the rectangle
	// Calculate new world position

	float2 dist2D = float2(dot(dir, left), dot(dir, up));

	float rectHalfSize = float2(halfWidth, halfHeight);
	dist2D = clamp ( dist2D , - rectHalfSize , rectHalfSize );
	return planeOrigin + dist2D.x * left + dist2D.y * up;
}

float AreaLightRect(RectAreaLight rectLight, float3 worldPos, float3 worldNormal)
{
    float illuminance = 0.0f;
	
	if(dot(worldPos - rectLight.LightPos, rectLight.PlaneNormal) > 0)
    {
        float clampCosAngle = 0.001 + saturate(dot(worldNormal, rectLight.PlaneNormal));
		// clamp d0 to the positive hemisphere of surface norma
        float3 d0 = normalize(-rectLight.PlaneNormal + worldNormal * clampCosAngle);
		// clamp d1 to the negative hemisphere of light plane normal
        float3 d1 = normalize(worldNormal - rectLight.PlaneNormal * clampCosAngle);
        float3 dh = normalize(d0 + d1);
	
        float lightHalfWidth = rectLight.LightWidth * 0.5;
        float lightHalfHeight = rectLight.LightHeight * 0.5;

        float3 ph = RayPlaneIntersect(worldPos, dh, rectLight.LightPos, rectLight.PlaneNormal);
        ph = closestPointRect(ph, rectLight.LightPos, rectLight.LightLeft, rectLight.LightUp, lightHalfWidth, lightHalfHeight);
	
        float3 p0 = rectLight.LightPos + rectLight.LightLeft * -lightHalfWidth + rectLight.LightUp * lightHalfHeight;
        float3 p1 = rectLight.LightPos + rectLight.LightLeft * -lightHalfWidth + rectLight.LightUp * -lightHalfHeight;
        float3 p2 = rectLight.LightPos + rectLight.LightLeft * lightHalfWidth + rectLight.LightUp * -lightHalfHeight;
        float3 p3 = rectLight.LightPos + rectLight.LightLeft * lightHalfWidth + rectLight.LightUp * lightHalfHeight;
	
        float solidAngle = RectangleSolidAngle(worldPos, p0, p1, p2, p3);
	
        float3 unormLightVector = ph - worldPos;
        float sqrDist = dot(unormLightVector, unormLightVector);
        float3 L = normalize(unormLightVector);
        illuminance = solidAngle * saturate(dot(worldNormal, L));
    }
	
    float3 result = illuminance * rectLight.Color * rectLight.Intensity;
    return result;
}

