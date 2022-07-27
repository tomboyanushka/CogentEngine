#define GROUP_SIZE_X 32
#define GROUP_SIZE_Y 32

RWTexture2D<float4> Result : register(u0);

float rand(float2 co)
{
    return 0.5 + (frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453)) * 0.5;
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void CSInit(uint3 id : SV_DispatchThreadID)
{

}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 ThreadID : SV_DispatchThreadID)
{
    // Init
    float rnd = rand(ThreadID.xy);
    Result[ThreadID.xy] = rnd > .75 ? float4(1, 1, 1, 1) : float4(0, 0, 0, 0);

    int sum = 0;
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            if (x == 0 && y == 0) continue;

            if (Result[ThreadID.xy + float2(x, y)].x > 0)
                sum++;
        }
    }

    if (Result[ThreadID.xy].x > 0)
    {
        Result[ThreadID.xy] = (sum == 2 || sum == 3) ? float4(1, 1, 1, 1) : float4(0, 0, 0, 1);
    }
    else
    {
        Result[ThreadID.xy] = (sum == 3) ? float4(1, 1, 1, 1) : float4(0, 0, 0, 1);
    }
}