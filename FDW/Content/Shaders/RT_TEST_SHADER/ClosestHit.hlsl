#include "SameShadersStructs.hlsli"

struct Attributes {
    float2 bary;
};

[shader("closesthit")]
void ClosestHit(inout MyPayload payload, Attributes attr)
{
    payload.color = float4(1, 0, 0, 1); // RED
}
