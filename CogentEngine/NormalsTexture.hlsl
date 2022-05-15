#include "Common.hlsli"

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION;
};

//Texture2D normalTexture : register(t0);

float4 main(VertexToPixel input) : SV_TARGET
{
    float3 output;
    //input.normal = calculateNormalFromMap(input.uv, input.normal, input.tangent, normalTexture, )
    output = input.normal;
	return float4(output, 1.0f);
}