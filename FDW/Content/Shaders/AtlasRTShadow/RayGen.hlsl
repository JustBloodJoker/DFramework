#include "SameShadersStructs.hlsli"
#include "Structures.hlsli"
#include "Utilits.hlsli"

ConstantBuffer<ShadowAtlasParams> AtlasData : register(b0);
RaytracingAccelerationStructure SceneBVH : register(t0);
StructuredBuffer<LightStruct> Lights : register(t1);
StructuredBuffer<LightAtlasMeta> LightAtlasMetas : register(t2);
Texture2D<uint> TexelToLight : register(t3);
Texture2D<float4> DepthBuffer : register(t4);
Texture2D<float4> GBufferNormal : register(t5);
SamplerState samplerLinearClamp : register(s0);

RWTexture2D<float> ShadowAtlasUAV : register(u0);

float TraceShadowRay(float3 origin, float3 normal, float3 dir, float dist)
{
    RayDesc ray;
    
    float NdotL = dot(normal, dir);
    float baseBias = 0.0002f; 
    float bias = clamp(0.0002f * sqrt(1.0f - NdotL * NdotL) / max(NdotL, 1e-5f), 0.0002f, 0.005f);

    ray.Origin = origin + normal * bias; 
    
    ray.Direction = dir;
    ray.TMin = 0.0f;
    ray.TMax = dist - bias;

    ShadowPayload payload;
    payload.Visible = 1.0f;
    
    TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 1, 0, ray, payload);

    return payload.Visible;
}

[shader("raygeneration")]
void RayGen()
{
    uint2 pixel = DispatchRaysIndex().xy;
    if (pixel.x >= AtlasData.AtlasWidth || pixel.y >= AtlasData.AtlasHeight) 
        return;

    uint texelLookup = TexelToLight.Load(int3(pixel,0));
    if (texelLookup >= 0xFFu) {
        ShadowAtlasUAV[pixel] = 1.0f;
        return;
    }

    LightAtlasMeta meta = LightAtlasMetas[texelLookup];
    LightStruct light   = Lights[meta.LightIndex];

    uint2 localPixel = uint2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    uint2 dtPixel = pixel - localPixel;
    float2 uvLocal = (float2(dtPixel) + 0.5f) / float2(meta.AtlasWidth, meta.AtlasHeight);

    float screenU = lerp(meta.ScreenMinU, meta.ScreenMaxU, uvLocal.x);
    float screenV = lerp(meta.ScreenMinV, meta.ScreenMaxV, uvLocal.y);
    
    float2 screenPos = float2(screenU * AtlasData.ScreenWidth, screenV * AtlasData.ScreenHeight);

    uint2 gBuffersPixel = uint2(floor(screenPos)); 
    
    gBuffersPixel.x = min(gBuffersPixel.x, AtlasData.ScreenWidth - 1);
    gBuffersPixel.y = min(gBuffersPixel.y, AtlasData.ScreenHeight - 1);
    
    float2 uv = (float2(gBuffersPixel) + 0.5f) / float2(AtlasData.ScreenWidth, AtlasData.ScreenHeight);
    float3 worldPos = ReconstructWorldPosition(uv, DepthBuffer.Load(int3(gBuffersPixel,0)).x, AtlasData.InverseViewProjectionMatrix);
    float3 normal   = normalize(GBufferNormal.Load(int3(gBuffersPixel,0)).xyz);

    float visible = 1.0f;

    if (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE)
    {
        float3 lightDir = -normalize(light.Direction);
        const float INF_DIST = 1e6f;
        visible = TraceShadowRay(worldPos, normal, lightDir, INF_DIST);
    }
    else if (light.LightType == LIGHT_POINT_LIGHT_ENUM_VALUE)
    {
        float3 toLight = light.Position - worldPos;
        float dist = length(toLight);
        if (dist <= 0.0f) {
            visible = 0.0f;
        } else {
            if (light.AttenuationRadius > 0.0f && dist > light.AttenuationRadius)
            {
                visible = 0.0f;
            }
            else
            {
                float3 dir = toLight / dist;
                visible = TraceShadowRay(worldPos, normal, dir, dist);
            }
        }
    }
    else if (light.LightType == LIGHT_SPOT_LIGHT_ENUM_VALUE)
    {
        float3 toLight = light.Position - worldPos;
        float dist = length(toLight);
        if (dist <= 0.0f) {
            visible = 0.0f;
        } else
        {
            float3 L = normalize(worldPos - light.Position);
            float3 spotDir = normalize(light.Direction);
            float cosAngle = dot(spotDir, L);
            float cosOuter = cos(light.OuterConeAngle);
            if ( (light.AttenuationRadius > 0.0f && dist > light.AttenuationRadius) || cosAngle < cosOuter)
            {
                visible = 0.0f;
            } else 
            {
                float3 dir = normalize(toLight);
                visible = TraceShadowRay(worldPos, normal,dir, dist);
            }
            
        }
    }
    else
    {
        //todo Rect light shadow
        visible = 1.0f;
    }

    ShadowAtlasUAV[pixel] = visible;
}