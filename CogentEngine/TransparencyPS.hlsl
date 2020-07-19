#include "Lighting.hlsli"
cbuffer externalData : register(b0)
{
	DirectionalLight dirLight;
	float3 cameraPosition;
	float blendAmount;
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
	float3 albedo = diffuseTexture.Sample(basicSampler, input.uv).rgb;
	float4 color = float4(1,1,1,1);

	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	input.normal = calculateNormalFromMap(input.uv, input.normal, input.tangent);

	float3 dirToLight = normalize(-dirLight.Direction);
	float NdotL = dot(input.normal, dirToLight);
	NdotL = saturate(NdotL);
	float intensity = NdotL;
	//float spec = calculateSpecular(input.normal, input.worldPos, dirToLight, cameraPosition) * roughness;
	float3 totalLight = dirLight.DiffuseColor.rgb * NdotL + dirLight.AmbientColor.rgb;
	color = color * float4(totalLight,1);
	color.a = 0.5f;

	return color;
}
