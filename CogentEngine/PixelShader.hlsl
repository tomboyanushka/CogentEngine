#include "Lighting.hlsli"
cbuffer externalData : register(b0)
{
	DirectionalLight dirLight;
	float3 cameraPosition;
}


struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 position			: SV_POSITION;
	float3 normal			: NORMAL;
	float2 uv				: TEXCOORD;
	float3 tangent			: TANGENT;
	float3 worldPos			: POSITION;
};

//float calculateSpecular(float3 normal, float3 worldPos, float3 dirToLight, float3 camPos)
//{
//	float3 dirToCamera = normalize(camPos - worldPos);
//	float3 halfwayVector = normalize(dirToLight + dirToCamera);
//	//float3 refl = reflect(-dirToLight, normal);
//	float shininess = 64;
//	//float spec = pow(saturate(dot(dirToCamera, refl)), shininess);
//	return shininess == 0 ? 0.0f : pow(max(dot(halfwayVector, normal), 0), shininess);
//	//return spec;
//}

float4 main(VertexToPixel input) : SV_TARGET
{
	// Renormalize any interpolated normals
	input.normal = normalize(input.normal);
	float3 dirToLight = normalize(-dirLight.Direction);
	float NdotL = dot(input.normal, dirToLight);
	NdotL = saturate(NdotL);
	//float spec = calculateSpecular(input.normal, input.worldPos, dirToLight, cameraPosition) * roughness;
	float3 totalLight = dirLight.DiffuseColor * NdotL + dirLight.AmbientColor;
	return float4(totalLight,1);
}