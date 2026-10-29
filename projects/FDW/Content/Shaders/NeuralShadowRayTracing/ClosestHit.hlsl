#include "SameShadersStructs.hlsli"

struct Attributes { float2 bary; };

[shader("closesthit")]
void ClosestHit(inout NeuralShadowPayload payload, Attributes)
{
    payload.Blocked = 1.0f;
    payload.Distance = RayTCurrent();
}
