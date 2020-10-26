//THIS IS THE QUAD
struct VertexToPixel
{
	float4 position				:	SV_POSITION;
	float2 uv					:	TEXCOORD0;
};

VertexToPixel main(uint id : SV_VERTEXID)
{
	VertexToPixel output;

	output.uv = float2(
		(id << 1) & 2,
		id & 2);

	output.position = float4(output.uv, 0, 1);
	output.position.x = output.position.x * 2 - 1;
	output.position.y = output.position.y * -2 + 1;
	return output;
}