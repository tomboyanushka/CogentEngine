#include "Lighting.hlsli"
#define MAX_LIGHTS 1

cbuffer externalData : register(b0)
{
	DirectionalLight dirLight;
	PointLight pointLight[MAX_LIGHTS];
    SphereAreaLight sphereLight[MAX_LIGHTS];
	float3 cameraPosition;
	int pointLightCount;
}

float3 calculateNormalFromMap(float2 uv, float3 normal, float3 tangent, Texture2D normalTexture, SamplerState basicSampler)
{
    float3 normalFromTexture = normalTexture.Sample(basicSampler, uv).xyz;
    float3 unpackedNormal = normalFromTexture * 2.0f - 1.0f;
    float3 N = normal;
    float3 T = normalize(tangent - N * dot(tangent, N));
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    return normalize(mul(unpackedNormal, TBN));
}