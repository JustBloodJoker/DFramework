#include "SameShadersStructs.hlsli"

RaytracingAccelerationStructure Scene : register(t0);
RWTexture2D<float4> Output : register(u0);

[shader("raygeneration")]
void RayGen()
{
    uint2 dispatchID = DispatchRaysIndex().xy;
    uint2 dispatchDim = DispatchRaysDimensions().xy;

    float2 uv = (dispatchID + 0.5f) / dispatchDim;
    float3 origin = float3(0, 0, -5);
    float3 dir = normalize(float3(uv * 2.0f - 1.0f, 1.0f));

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.001f;
    ray.TMax = 10000.0f;

    MyPayload payload = { float4(0, 0, 0, 1) };
    TraceRay(Scene,
        RAY_FLAG_NONE,
        0xFF,
        0, 1, 0,
        ray, payload);

    Output[dispatchID] = payload.color;
}
