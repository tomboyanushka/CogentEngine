
RWTexture2D<float4> myTexture : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 ThreadID : SV_DispatchThreadID)
{
    myTexture[ThreadID.xy] = float4(1, 0, 0, 1);
}