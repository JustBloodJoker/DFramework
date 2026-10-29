#include "RS.hlsl"
#include "Structures.hlsli"
#include "Utilits.hlsli"

StructuredBuffer<float> HardLow : register(t0);
StructuredBuffer<float> Descriptor : register(t1);
StructuredBuffer<LightStruct> Lights : register(t2);
StructuredBuffer<float> DmlOutput : register(t3);
Texture2D<float4> DepthBuffer : register(t4);
Texture2D<float4> NormalBuffer : register(t5);
RWStructuredBuffer<float> Base : register(u0);
RWStructuredBuffer<float> Hard : register(u1);
RWStructuredBuffer<uint> FrameValidity : register(u2);
RWStructuredBuffer<float> Visibility : register(u3);
RWStructuredBuffer<uint> OutputValidity : register(u4);

struct Params
{
    uint ScreenWidth;
    uint ScreenHeight;
    uint LowWidth;
    uint LowHeight;
    uint RegionCountX;
    uint RegionCountY;
    uint LightIndex;
    uint LightCount;
    matrix InverseViewProjectionMatrix;
};
ConstantBuffer<Params> Params : register(b0);
cbuffer Pass : register(b1) { uint PassIndex; }

float LowAt(int2 coordinate)
{
    coordinate = clamp(coordinate, int2(0, 0), int2(Params.LowWidth - 1u, Params.LowHeight - 1u));
    return HardLow[uint(coordinate.x) + uint(coordinate.y) * Params.LowWidth];
}

float SampleHardVisibility(uint2 pixel)
{
    float2 source = (float2(pixel) + 0.5f) / 4.0f - 0.5f;
    int2 base = int2(floor(source));
    float2 fraction = frac(source);
    float top = lerp(LowAt(base), LowAt(base + int2(1, 0)), fraction.x);
    float bottom = lerp(LowAt(base + int2(0, 1)), LowAt(base + int2(1, 1)), fraction.x);
    return lerp(top, bottom, fraction.y);
}

[numthreads(8, 8, 1)]
void CS(uint3 dispatchId : SV_DispatchThreadID)
{
    uint2 pixel = dispatchId.xy;
    if (pixel.x >= Params.ScreenWidth || pixel.y >= Params.ScreenHeight) {
        return;
    }
    
    uint index = pixel.x + pixel.y * Params.ScreenWidth;
    uint plane = Params.ScreenWidth * Params.ScreenHeight;
    if (PassIndex == 0u) {
        float hard = SampleHardVisibility(pixel);
        
        Hard[index] = hard;
        
        float depth = DepthBuffer.Load(int3(pixel, 0)).r;
        float3 normal = NormalBuffer.Load(int3(pixel, 0)).xyz;
        float normalLength = length(normal);
        
        bool valid = FrameValidity[index] == 0u && isfinite(depth) && depth >= 0.0f && depth < 1.0f &&
            all(isfinite(normal)) && normalLength > 1e-4f && Params.LightIndex < Params.LightCount;
        
        float3 position = 0.0f;
        
        if (valid) {
            float2 uv = (float2(pixel) + 0.5f) / float2(Params.ScreenWidth, Params.ScreenHeight);
            position = ReconstructWorldPosition(uv, depth, Params.InverseViewProjectionMatrix);
            valid = all(isfinite(position));
        }
        
        LightStruct light = Lights[min(Params.LightIndex, Params.LightCount - 1u)];
        float3 vector = light.Position - position;
        float distance = length(vector);
        valid = valid && (light.LightType == 0u || light.LightType == 1u) && distance > 1e-6f;
        
        float radius = max(light.SourceRadius, 0.0f) *
            (isfinite(light.ShadowSoftness) ? clamp(light.ShadowSoftness, 0.0f, 4.0f) : 1.0f);
        
        Base[0u * plane + index] = valid ? hard : 1.0f;
        Base[1u * plane + index] = valid ? normal.x / normalLength : 0.0f;
        Base[2u * plane + index] = valid ? normal.y / normalLength : 0.0f;
        Base[3u * plane + index] = valid ? normal.z / normalLength : 0.0f;
        Base[4u * plane + index] = valid ? vector.x / distance : 0.0f;
        Base[5u * plane + index] = valid ? vector.y / distance : 0.0f;
        Base[6u * plane + index] = valid ? vector.z / distance : 0.0f;
        Base[7u * plane + index] = valid ? distance / 24.0f : 0.0f;
        Base[8u * plane + index] = valid ? radius / 20.0f : 0.0f;
        FrameValidity[index] = valid ? 0u : 1u;
        return;
    }

    uint2 region = pixel / 64u;
    uint2 local = pixel % 64u;
    uint regionIndex = region.x + region.y * Params.RegionCountX;
    uint workIndex = regionIndex * Params.LightCount + Params.LightIndex;
    uint outputIndex = workIndex * 4096u + local.x + local.y * 64u;
    float value = DmlOutput[index];
    bool valid = FrameValidity[index] == 0u && isfinite(value);
    Visibility[outputIndex] = valid ? saturate(value) : Hard[index];
    OutputValidity[outputIndex] = valid ? 0u : 1u;
}
