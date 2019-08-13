struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float3 sampleDir	: TEXCOORD;
};


// Texture-related variables
TextureCube skyTexture		: register(t0);
SamplerState basicSampler	: register(s0);


// Entry point for this pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
	return skyTexture.Sample(basicSampler, input.sampleDir);
}