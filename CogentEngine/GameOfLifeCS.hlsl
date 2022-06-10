[numthreads(1, 1, 1)]
float4 main(uint3 DTid : SV_DispatchThreadID)
{
    return float4(1, 0, 0, 0);
}