#include "SameShadersStructs.hlsli"

[shader("miss")]
void Miss(inout NeuralShadowPayload payload)
{
    payload.Blocked = 0.0f;
    payload.Distance = 1.0f;
}
