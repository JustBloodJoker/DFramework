#ifndef FDW_RT_SHADOW_BIAS_HLSLI
#define FDW_RT_SHADOW_BIAS_HLSLI

void RTShadowComputeBias(
    float3 normal,
    float3 direction,
    float pixelWorldSize,
    out float normalBias,
    out float rayEpsilon)
{
    float ndotl = dot(normal, direction);
    float slope = 0.0002f * sqrt(max(0.0f, 1.0f - ndotl * ndotl)) / max(ndotl, 1e-5f);
    normalBias = clamp(max(clamp(slope, 0.003f, 0.07f), 0.65f * pixelWorldSize), 0.03f, 1.75f);
    rayEpsilon = max(0.03f, 0.7f * normalBias);
}

#endif
