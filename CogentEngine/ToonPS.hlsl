#include "Lighting.hlsli"
cbuffer externalData : register(b0)
{
	DirectionalLight dirLight;
	float3 cameraPosition;
}


struct VertexToPixel
{
	float4 position			: SV_POSITION;
	float3 normal			: NORMAL;
	float2 uv				: TEXCOORD;
	float3 tangent			: TANGENT;
	float3 worldPos			: POSITION;
};

Texture2D diffuseTexture		: register(t0);
Texture2D normalTexture			: register(t1);
SamplerState basicSampler		: register(s0);

float3 calculateNormalFromMap(float2 uv, float3 normal, float3 tangent)
{
	float3 normalFromTexture = normalTexture.Sample(basicSampler, uv).xyz;
	float3 unpackedNormal = normalFromTexture * 2.0f - 1.0f;
	float3 N = normal;
	float3 T = normalize(tangent - N * dot(tangent, N));
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
	return normalize(mul(unpackedNormal, TBN));
}

float4 main(VertexToPixel input) : SV_TARGET
{
	float3 diffuse = diffuseTexture.Sample(basicSampler, input.uv).rgb;
	float3 color = float3(1,1,1);

	//---DIFFUSE LIGHTING---
	input.normal = normalize(input.normal);
	float3 worldNormal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	input.normal = calculateNormalFromMap(input.uv, input.normal, input.tangent);
	float3 dirToLight = normalize(-dirLight.Direction);
	float NdotL = dot(input.normal, dirToLight);
	NdotL = saturate(NdotL);
	float3 totalLight = dirLight.DiffuseColor.rgb * NdotL + dirLight.AmbientColor.rgb;

	//---SPECULAR REFLECTION
	//properties
	float4 specularColor = float4(0.9, 0.9, 0.9, 1);
	float glossiness = 32;
	float3 viewDir = normalize(cameraPosition - input.worldPos);
	float3 halfVector = normalize(dirToLight + viewDir);
	float NdotH = dot(worldNormal, halfVector);
	float lightIntensity = smoothstep(0, 0.01, NdotL);
	float specularIntensity = pow(NdotH * lightIntensity, glossiness * glossiness);
	float specularIntensitySmooth = smoothstep(0.005, 0.01, specularIntensity);
	float4 specular = specularIntensitySmooth * specularColor;

	//---RIM LIGHTING---
	//properties
	float4 rimColor = float4(1, 1, 1, 1);
	float rimAmount = 0.716;
	float rimThreshold = 0.1;
	float rimDot = 1 - max(dot(viewDir, worldNormal), 0.0);
	float rimIntensity = rimDot * pow(NdotL, rimThreshold);
	rimIntensity = smoothstep(rimAmount - 0.01, rimAmount + 0.01, rimIntensity);
	float4 finalRim = rimIntensity * rimColor;

	//---TOONSHADING--- 
	//Discretize the intensity, based on a few cutoff points
	lightIntensity = NdotL;
	if (lightIntensity > 0.95)
		color = 1 * color;
	else if (lightIntensity > 0.5)
		color = 0.7 * color;
	else if (lightIntensity > 0.15)
		color = 0.1 * color;
	else
		color = 0 * color;
	//lightIntensity = NdotL > 0 ? 1 : 0;

	return float4(color * lightIntensity * diffuse * (totalLight + finalRim + specular), 1);
}