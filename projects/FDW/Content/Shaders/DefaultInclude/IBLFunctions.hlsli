#ifndef _IBL_FUNCTIONS_HLSLI_
#define _IBL_FUNCTIONS_HLSLI_

#include "Utilits.hlsli"

float RadicalInverseVdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

float2 Hammersley(uint i, uint n)
{
    return float2(float(i) / float(n), RadicalInverseVdC(i));
}

float3 SampleCosineHemisphere(float2 xi)
{
    float phi = 2.0f * PI * xi.x;
    float cosTheta = sqrt(1.0f - xi.y);
    float sinTheta = sqrt(xi.y);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float3 CubemapDirectionFromFaceUV(uint face, float u, float v)
{
    switch (face) {
        case 0: return normalize(float3(1.0f, v, -u));
        case 1: return normalize(float3(-1.0f, v, u));
        case 2: return normalize(float3(u, 1.0f, -v));
        case 3: return normalize(float3(u, -1.0f, v));
        case 4: return normalize(float3(u, v, 1.0f));
        default: return normalize(float3(-u, v, -1.0f));
    }
}


#endif


