#include "SameShadersStructs.hlsli"
#include "Structures.hlsli"
#include "Utilits.hlsli"

RaytracingAccelerationStructure SceneBVH : register(t0);
StructuredBuffer<LightStruct> Lights : register(t1);
Texture2D<float4> DepthBuffer : register(t2);
Texture2D<float4> NormalBuffer : register(t3);
RWStructuredBuffer<float> HardLow : register(u0);
RWStructuredBuffer<float> Descriptor : register(u1);
RWStructuredBuffer<uint> Validity : register(u2);

struct NeuralRayParameters
{
    uint ScreenWidth;
    uint ScreenHeight;
    uint LowWidth;
    uint LowHeight;
    uint LightIndex;
    uint LightCount;
    uint PassIndex;
    uint Padding;
    matrix InverseViewProjectionMatrix;
};
ConstantBuffer<NeuralRayParameters> Params : register(b0);

static const float2 OFFSETS[9] = {
    float2(-0.6f, -0.6f), float2(0.0f, -0.6f), float2(0.6f, -0.6f),
    float2(-0.6f,  0.0f), float2(0.0f,  0.0f), float2(0.6f,  0.0f),
    float2(-0.6f,  0.6f), float2(0.0f,  0.6f), float2(0.6f,  0.6f)
};

float3 WorldPosition(uint2 pixel)
{
    float depth = DepthBuffer.Load(int3(pixel, 0)).r;
    float2 uv = (float2(pixel) + 0.5f) / float2(Params.ScreenWidth, Params.ScreenHeight);
    return ReconstructWorldPosition(uv, depth, Params.InverseViewProjectionMatrix);
}

bool Receiver(uint2 pixel, out float3 position, out float3 normal, out float pixelWorldSize)
{
    float depth = DepthBuffer.Load(int3(pixel, 0)).r;
    
    normal = NormalBuffer.Load(int3(pixel, 0)).xyz;
    if (!isfinite(depth) || depth < 0.0f || depth >= 1.0f ||
        !all(isfinite(normal)) || dot(normal, normal) < 1e-8f) {
        
        position = 0.0f;
        normal = float3(0.0f, 1.0f, 0.0f);
        pixelWorldSize = 0.0f;
        return false;
    }
    
    normal = normalize(normal);
    position = WorldPosition(pixel);
    
    uint2 limit = uint2(Params.ScreenWidth - 1u, Params.ScreenHeight - 1u);
    
    uint2 left = uint2(pixel.x > 0u ? pixel.x - 1u : pixel.x, pixel.y);
    uint2 right = uint2(min(pixel.x + 1u, limit.x), pixel.y);
    uint2 up = uint2(pixel.x, pixel.y > 0u ? pixel.y - 1u : pixel.y);
    uint2 down = uint2(pixel.x, min(pixel.y + 1u, limit.y));
    
    float3 dxForward = WorldPosition(right) - position;
    float3 dxBackward = position - WorldPosition(left);
    float3 dyForward = WorldPosition(down) - position;
    float3 dyBackward = position - WorldPosition(up);
    float3 dx = dot(dxForward, dxForward) < dot(dxBackward, dxBackward) ? dxForward : dxBackward;
    float3 dy = dot(dyForward, dyForward) < dot(dyBackward, dyBackward) ? dyForward : dyBackward;
    
    pixelWorldSize = max(length(dx), length(dy));
    
    float3 geometric = cross(dy, dx);
    float geometricLengthSq = dot(geometric, geometric);
    if (geometricLengthSq > 1e-10f) {
        geometric *= rsqrt(geometricLengthSq);
        normal = dot(geometric, normal) < 0.0f ? -geometric : geometric;
    }
    
    return all(isfinite(position));
}

void TraceToEmitter(float3 receiver, float3 receiverNormal, float pixelWorldSize, float3 emitter,
    out float blocked, out float distanceFraction)
{
    float3 vector = emitter - receiver;
    float distance = length(vector);
    blocked = 0.0f;
    distanceFraction = 1.0f;
    if (!isfinite(distance) || distance <= 1e-6f) {
        return;
    }
    
    RayDesc ray;
    float normalBias = clamp(max(0.01f, pixelWorldSize * 0.65f), 0.01f, 1.75f);
    float epsilon = max(0.001f, normalBias * 0.7f);
    ray.Direction = vector / distance;
    ray.Origin = receiver + receiverNormal * normalBias + ray.Direction * epsilon;
    ray.TMin = epsilon;
    ray.TMax = distance - normalBias - epsilon;
    if (ray.TMax <= ray.TMin) {
        return;
    }
    
    NeuralShadowPayload payload;
    payload.Blocked = 0.0f;
    payload.Distance = 1.0f;
    TraceRay(SceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, 0, 1, 0, ray, payload);
    
    blocked = payload.Blocked;
    distanceFraction = payload.Blocked > 0.5f ? saturate(payload.Distance / max(distance, 1e-6f)) : 1.0f;
}

[shader("raygeneration")]
void RayGen()
{
    uint2 pixel = DispatchRaysIndex().xy;
    if (pixel.x >= Params.ScreenWidth || pixel.y >= Params.ScreenHeight || Params.LightIndex >= Params.LightCount) {
        return;
    }
    
    uint index = pixel.x + pixel.y * Params.ScreenWidth;
    uint plane = Params.ScreenWidth * Params.ScreenHeight;
    
    LightStruct light = Lights[Params.LightIndex];
    float radius = max(light.SourceRadius, 0.0f) * (isfinite(light.ShadowSoftness) ? clamp(light.ShadowSoftness, 0.0f, 4.0f) : 1.0f);
    
    if (Params.PassIndex == 0u) {
        float3 receiver;
        float3 receiverNormal;
        float pixelWorldSize;
        bool valid = Receiver(pixel, receiver, receiverNormal, pixelWorldSize);
        Validity[index] = valid ? 0u : 1u;
        bool supported = valid && (light.LightType == 0u || light.LightType == 1u) && all(isfinite(light.Position)) && isfinite(radius);
        
        [unroll]
        for (uint sample = 0u; sample < 9u; ++sample) {
            float blocked = 0.0f;
            float distanceFraction = 1.0f;
            
            if (supported) {
                float3 emitter = light.Position + float3(OFFSETS[sample].x * radius, 0.0f, OFFSETS[sample].y * radius);
                TraceToEmitter(receiver, receiverNormal, pixelWorldSize, emitter, blocked, distanceFraction);
            }
            
            Descriptor[sample * plane + index] = blocked;
            Descriptor[(9u + sample) * plane + index] = distanceFraction;
        }
        return;
    }

    if (Params.PassIndex == 1u && (pixel.x & 3u) == 0u && (pixel.y & 3u) == 0u) {
        uint2 low = pixel / 4u;
        float sum = 0.0f;
        uint count = 0u;

        [unroll]
        for (uint oy = 0u; oy < 4u; ++oy) {
            
            [unroll]
            for (uint ox = 0u; ox < 4u; ++ox) {
                uint2 samplePixel = pixel + uint2(ox, oy);
                if (samplePixel.x >= Params.ScreenWidth || samplePixel.y >= Params.ScreenHeight) {
                    continue;
                }
                float3 sampleReceiver;
                float3 sampleNormal;
                float samplePixelWorldSize;
                if (!Receiver(samplePixel, sampleReceiver, sampleNormal, samplePixelWorldSize)) {
                    continue;
                }
                float blocked;
                float distanceFraction;
                TraceToEmitter(sampleReceiver, sampleNormal, samplePixelWorldSize, light.Position, blocked, distanceFraction);
                sum += 1.0f - blocked;
                count++;
            }

        }
        HardLow[low.x + low.y * Params.LowWidth] = count != 0u ? sum / float(count) : 1.0f;
    }
}
