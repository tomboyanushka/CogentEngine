cbuffer Data : register(b0)
{
    float pixelWidth;
    float pixelHeight;
    float blurAmount;
    float focusPlaneZ;
    float zFar;
    float zNear;
    float pad0;
    float pad1;
}

struct VertexToPixel
{
	float4 position					: SV_POSITION;
	float2 uv						: TEXCOORD0;
};

Texture2D Pixels					: register(t0);
Texture2D DepthBuffer				: register(t1);
SamplerState Sampler				: register(s0);

float CalcLinearZ(float depth) //Screenspace Z
{
	float2 projectionConstants;
	projectionConstants.x = zFar / (zFar - zNear);
	projectionConstants.y = (-zFar * zNear) / (zFar - zNear);
	float linearZ = projectionConstants.y / (depth - projectionConstants.x);
	return linearZ;

}

float4 main(VertexToPixel input) : SV_TARGET
{

	float4 totalColor = float4(0, 0, 0, 0);
	float4 finalColor = float4(0, 0, 0, 0);
	uint numSamples = 0;

    float width = 1 / 1920.0;
    float height = 1 / 1061.0;
	
	int blurAmount = 3.0f;
	float depth = DepthBuffer.Sample(Sampler, input.uv).r;
	float linearZ = CalcLinearZ(depth);
	//float3 WorldZ = WorldPosFromDepth(DepthBuffer.Sample(Sampler, input.uv).r, input.uv);
	for (int x = -blurAmount; x <= blurAmount; x++)
	{
		for (int y = -blurAmount; y <= blurAmount; y++)
		{
			float2 uv = input.uv + float2(x * width, y * height);
			totalColor += Pixels.Sample(Sampler, uv);

			numSamples++;
		}
	}
    finalColor = totalColor / numSamples;
	//if (linearZ > focusPlaneZ + 2 || linearZ < focusPlaneZ - 1)
	//{
	//	finalColor = totalColor / numSamples;
	//}
	//else
	//{
	//	finalColor = Pixels.Sample(Sampler, input.uv);
	//	numSamples = 1;
	//}

	return finalColor;
}