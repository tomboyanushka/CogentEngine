
struct VertexToPixel
{
	float4 position			: SV_POSITION;
	float3 normal			: NORMAL;
	float2 uv				: TEXCOORD;
	float3 tangent			: TANGENT;
	float3 worldPos			: POSITION;
};

float4 main(VertexToPixel input) : SV_TARGET
{
	float3 lineColor = float3(0, 0, 0);

	return float4(lineColor,1);
}