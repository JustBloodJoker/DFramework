#include "SameShadersStructs.hlsli"

[shader("miss")]
void Miss(inout ShadowPayload payload)
{
    payload.Visible = 1.0;
}