#include "Common.hlsli"

struct VertexToPixel
{
	float4 position			: SV_POSITION;
	float3 normal			: NORMAL;
	float2 uv				: TEXCOORD;
	float3 tangent			: TANGENT;
	float3 worldPos			: POSITION;
};

Texture2D diffuseTexture			: register(t0);
Texture2D normalTexture				: register(t1);
Texture2D metalTexture				: register(t2);
Texture2D roughnessTexture			: register(t3);

TextureCube skyIrradianceTexture	: register(t4);
TextureCube skyPrefilterTexture		: register(t5);
Texture2D brdfLookUpTexture			: register(t6);

SamplerState basicSampler			: register(s0);

//float3 calculateNormalFromMap(float2 uv, float3 normal, float3 tangent)
//{
//	float3 normalFromTexture = normalTexture.Sample(basicSampler, uv).xyz;
//	float3 unpackedNormal = normalFromTexture * 2.0f - 1.0f;
//	float3 N = normal;
//	float3 T = normalize(tangent - N * dot(tangent, N));
//	float3 B = cross(N, T);
//	float3x3 TBN = float3x3(T, B, N);
//	return normalize(mul(unpackedNormal, TBN));
//}

float3 PrefilteredColor(float3 viewDir, float3 normal, float roughness)
{
	const float MAX_REF_LOD = 8.0f;
	float3 R = reflect(-viewDir, normal);
	return skyPrefilterTexture.SampleLevel(basicSampler, R, roughness * MAX_REF_LOD).rgb;
}

float2 BrdfLUT(float3 normal, float3 viewDir, float roughness)
{
	float NdotV = dot(normal, viewDir);
	NdotV = max(NdotV, 0.0f);
	float2 uv = float2(NdotV, roughness);
	return brdfLookUpTexture.Sample(basicSampler, uv).rg;
}

float4 main(VertexToPixel input) : SV_TARGET
{
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	float4 surfaceColor = diffuseTexture.Sample(basicSampler, input.uv);

    
	input.normal = calculateNormalFromMap(input.uv, input.normal, input.tangent, normalTexture, basicSampler);

	// Sample the roughness map
	float roughness = roughnessTexture.Sample(basicSampler, input.uv).r;
	//roughness = lerp(0.2f, roughness, 1);

	// Sample the metal map 
	float metalness = metalTexture.Sample(basicSampler, input.uv).r;
	//metalness = lerp(0.0f, metalness, 1);

	// Specular color - Assuming albedo texture is actually holding specular color if metal == 1
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metalness);
	//float3 specColor = F0_NON_METAL.rrr;// , surfaceColor.rgb, 1);

	//float3 lightOne = surfaceColor.rgb * (light1.DiffuseColor * dirNdotL + light1.AmbientColor);
	//float3 lightTwo = surfaceColor.rgb * (light2.DiffuseColor * dirNdotL2 + light2.AmbientColor);
	// Total color for this pixel
	float3 totalColor = float3(0, 0, 0);

	//IBL calculations
	float3 viewDir = normalize(cameraPosition - input.worldPos);
	float3 prefilter = PrefilteredColor(viewDir, input.normal, roughness);
	float2 brdf = BrdfLUT(input.normal, viewDir, roughness);
	float3 irradiance = skyIrradianceTexture.Sample(basicSampler, input.normal).rgb;

	float3 dirPBR = DirLightPBR(dirLight, input.normal, input.worldPos, cameraPosition, roughness, metalness, surfaceColor.rgb, specColor, irradiance, prefilter, brdf);
	//float3 dirPBR = DirLightPBR(light1, input.normal, input.worldPos, cameraPosition, roughness, metalness, surfaceColor.rgb, specColor);
	//float3 pointPBR = PointLightPBR(light3, input.normal, input.worldPos, cameraPosition, roughness, metalness, surfaceColor.rgb, specColor);
	//float3 spotPBR = SpotLightPBR(light4, input.normal, input.worldPos, cameraPosition, roughness, metalness, surfaceColor.rgb, specColor);

	totalColor = dirPBR * dirLight.Intensity;
    clip(surfaceColor.a < 0.05f ? -1 : 1);
    
	float gamma = 2.4f;
    totalColor = abs(pow(totalColor, 1.0 / gamma));
	return float4(totalColor * surfaceColor.rgb, 1);
}
