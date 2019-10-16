
cbuffer externalData : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	//float lineThickness;
};


struct VertexShaderInput
{

	float3 position		: POSITION;     // XYZ position
	float3 normal		: NORMAL;        // Normals
	float2 uv           : TEXCOORD;     //UVs
	float3 tangent		: TANGENT;
};

struct VertexToPixel
{

	float4 position		: SV_POSITION;
	float3 normal		: NORMAL;
	float2 uv           : TEXCOORD;
	float3 tangent		: TANGENT;
	float3 worldPos		: POSITION;
};


VertexToPixel main(VertexShaderInput input)
{
	// Set up output struct
	VertexToPixel output;
	float lineThickness = 0.02f;
	matrix worldViewProj = mul(mul(world, view), projection);
	float4 original = mul(float4(input.position + input.normal * lineThickness , 1.0f), worldViewProj);
	//float4 original = mul(input.position, worldViewProj);
	
	//output.normal = mul(input.normal, (float3x3)world);
	//float4 normal = mul(input.normal, worldViewProj);
	// Take the correct "original" location and translate the vertex a little
	// bit in the direction of the normal to draw a slightly expanded object.
	// Later, we will draw over most of this with the right color, except the expanded
	// part, which will leave the outline that we want.
	output.position = original;// +(mul(1, normal));

	return output;
}