#include "SameShadersStructs.hlsli"
#include "Structures.hlsli"

ConstantBuffer<ShadowAtlasParams> AtlasData : register(b0);
RaytracingAccelerationStructure SceneBVH : register(t0);
StructuredBuffer<LightStruct> Lights : register(t1);
StructuredBuffer<LightAtlasMeta> LightAtlasMetas : register(t2);
Texture2D<uint> TexelToLight : register(t3);
Texture2D<float4> GBufferPos : register(t4);
Texture2D<float4> GBufferNormal : register(t5);
SamplerState samplerLinearClamp : register(s0);

RWTexture2D<float> ShadowAtlasUAV : register(u0);

float TraceShadowRay(float3 origin, float3 dir, float dist)
{
    RayDesc ray;
    ray.Origin = origin + dir * 0.001f;
    ray.Direction = dir;
    ray.TMin = 0.0f;
    ray.TMax = dist - 0.01f;

    ShadowPayload payload;
    payload.Visible = 1.0f;

    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    return payload.Visible;
}

[shader("raygeneration")]
void RayGen()
{
    uint2 pixel = DispatchRaysIndex().xy;
    if (pixel.x >= AtlasData.AtlasWidth || pixel.y >= AtlasData.AtlasHeight) 
        return;

    uint texelLookup = TexelToLight.Load(int3(pixel,0));
    if (texelLookup == 0xFFFFFFFFu) {
        ShadowAtlasUAV[pixel] = 1.0f;
        return;
    }

    LightAtlasMeta meta = LightAtlasMetas[texelLookup];
    LightStruct light   = Lights[meta.LightIndex];

    uint2 localPixel = uint2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    uint2 dtPixel = pixel - localPixel;
    float2 uvLocal = dtPixel / float2(meta.AtlasWidth, meta.AtlasHeight);
    
    //uint2 gBuffersPixel = uint2(uvLocal.x * AtlasData.ScreenWidth, uvLocal.y * AtlasData.ScreenHeight);

    ///////////////////     Texel snapping
    float2 screenPos = float2(uvLocal.x * AtlasData.ScreenWidth, uvLocal.y * AtlasData.ScreenHeight);
    uint2 gBuffersPixel = uint2(floor(screenPos + 0.5f)); // rounding to nearest texel
    // optional: clamp to valid range
    gBuffersPixel.x = min(gBuffersPixel.x, AtlasData.ScreenWidth - 1);
    gBuffersPixel.y = min(gBuffersPixel.y, AtlasData.ScreenHeight - 1);
    ////////////////////

    float3 worldPos = GBufferPos.Load(int3(gBuffersPixel,0)).xyz;
    float3 normal   = normalize(GBufferNormal.Load(int3(gBuffersPixel,0)).xyz);

    float visible = 1.0f;

    if (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE)
    {
        float3 lightDir = -normalize(light.Direction);
        const float INF_DIST = 1e6f;
        visible = TraceShadowRay(worldPos, lightDir, INF_DIST);
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
                visible = TraceShadowRay(worldPos, dir, dist);
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
                visible = TraceShadowRay(worldPos, dir, dist);
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