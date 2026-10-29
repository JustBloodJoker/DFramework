#include "SameShadersStructs.hlsli"
struct Attributes { float2 bary; };
[shader("closesthit")]
void ClosestHit(inout ShadowPayload payload, Attributes attributes)
{
    payload.Visible = 0.0f;
}
