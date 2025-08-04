#include "SameShadersStructs.hlsli"

[shader("miss")]
void Miss(inout MyPayload payload)
{
    payload.color = float4(0, 0, 0, 1); // Black background
}
