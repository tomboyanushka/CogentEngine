cbuffer Data : register(b0)
{

	float4x4 viewMatrixInv;
	float4x4 projMatrixInv;
	float focusPlaneZ;
	float scale;
	float zFar;
	float zNear;
	//float2 projectionConstants;


}

struct VertexToPixel
{
	float4 position					: SV_POSITION;
	float2 uv						: TEXCOORD0;
};

Texture2D Pixels					: register(t0);
Texture2D DepthBuffer				: register(t1);
SamplerState Sampler				: register(s0);

float UnpackFloat(float linearZ, float farZ)
{
	return linearZ * farZ;
}


float4 main(VertexToPixel input) : SV_TARGET
{
	//float scale = 0.1f;
	float linearZ = UnpackFloat(Pixels.Sample(Sampler, input.uv).a, 100);

// 0.105 * 100 = 10.5
float radius = (linearZ - focusPlaneZ) * scale;
	radius = (radius + 1) / 2;
	return float4(radius, 0, 0, 0);
}