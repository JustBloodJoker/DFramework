#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

struct PIXEL_OUTPUT {
    float4 result : SV_TARGET0;
};

Texture2D SRV_RTVTextures[2]    : register(t0);
Texture2D RenderOutput          : register(t2);
Texture2D MotionBuffer          : register(t3);
Texture2D SRV_DepthTextures[2]  : register(t4);

ConstantBuffer<TAAPassData> taaData : register(b0);

SamplerState pointSS  : register(s0);
SamplerState linearSS : register(s1);

float ReprojectCurrentDepthToPrevDepth(float2 uv, float currentDepth)
{
    float2 ndc = uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;

    float4 clipPos = float4(ndc, currentDepth, 1.0f);
    float4 worldPos = mul(clipPos, taaData.CurrentInvViewProjectionMatrix);
    worldPos /= max(abs(worldPos.w), 1e-6f);

    float4 prevClip = mul(float4(worldPos.xyz, 1.0f), taaData.PrevViewProjectionMatrix);
    return prevClip.z / max(prevClip.w, 1e-6f);
}

float3 ClipHistoryToAABB(float3 historyColor, float3 aabbMin, float3 aabbMax)
{
    float3 center = (aabbMax + aabbMin) * 0.5f;
    float3 extents = max((aabbMax - aabbMin) * 0.5f, float3(1e-5f, 1e-5f, 1e-5f));
    float3 offset = historyColor - center;
    float3 unit = offset / extents;
    float maxUnit = max(abs(unit.x), max(abs(unit.y), abs(unit.z)));

    if (maxUnit > 1.0f) {
        return center + offset / maxUnit;
    }

    return historyColor;
}

void GetNeighborhoodBounds(uint2 centerPixel, uint w, uint h, float3 centerColor, out float3 aabbMin, out float3 aabbMax)
{
    aabbMin = centerColor;
    aabbMax = centerColor;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            int2 samplePixel = clamp(
                int2(centerPixel) + int2(x, y),
                int2(0, 0),
                int2((int)w - 1, (int)h - 1)
            );

            float4 sampleColorPP = RenderOutput.Load(int3(samplePixel, 0));
            if (sampleColorPP.a == 3.0f) continue;

            float3 sampleColor = sampleColorPP.rgb;
            if (any(isnan(sampleColor)) || any(isinf(sampleColor))) continue;

            aabbMin = min(aabbMin, sampleColor);
            aabbMax = max(aabbMax, sampleColor);
        }
    }
}

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    float2 uv = vsOut.texCoord;

    float4 currentColorPP = RenderOutput.SampleLevel(pointSS, uv, 0).rgba;
    if(currentColorPP.a == 3.0f) {
        psOut.result = float4(0, 0, 0, 3.0f);
        return psOut;
    }
    float3 currentColor = currentColorPP.xyz;

    if (any(isnan(currentColor)) || any(isinf(currentColor))) {
        currentColor = float3(0.0f, 0.0f, 0.0f);
    }

    uint w, h;
    RenderOutput.GetDimensions(w, h);
    float2 texelSize = 1.0f / float2(w, h);
    uint2 pixelCoord = min(uint2(uv * float2(w, h)), uint2(w - 1, h - 1));

    float2 offsets[4] = { float2(0, -texelSize.y), float2(0, texelSize.y), float2(-texelSize.x, 0), float2(texelSize.x, 0) };

    float  closestDepth = SRV_DepthTextures[taaData.CurrentDepthBufferIndex].SampleLevel(pointSS, uv, 0).r;
    float2 bestVelocity = MotionBuffer.SampleLevel(pointSS, uv, 0).xy;
    float2 closestDepthUV = uv;

    [unroll]
    for(int i = 0; i < 4; i++) {
        float2 neighborUV = saturate(uv + offsets[i]);
        float neighborDepth = SRV_DepthTextures[taaData.CurrentDepthBufferIndex].SampleLevel(pointSS, neighborUV, 0).r;
        
        if (neighborDepth < closestDepth) { 
            closestDepth = neighborDepth;
            bestVelocity = MotionBuffer.SampleLevel(pointSS, neighborUV, 0).xy;
            closestDepthUV = neighborUV;
        }
    }

    float2 jitterCurrUV = float2( taaData.currentJitter.x * 0.5f, -taaData.currentJitter.y * 0.5f );
    float2 jitterPrevUV = float2( taaData.prevJitter.x * 0.5f, -taaData.prevJitter.y * 0.5f );
    float2 deltaJitterUV = jitterCurrUV - jitterPrevUV;
    
    float2 prevUV = uv - bestVelocity - deltaJitterUV;

    if (prevUV.x < 0.0f || prevUV.x > 1.0f || prevUV.y < 0.0f || prevUV.y > 1.0f) {
        psOut.result = float4(currentColor, 1.0f);
        return psOut;
    }

    float3 historyColor = SRV_RTVTextures[taaData.CurrentReaderIndex].SampleLevel(linearSS, prevUV, 0).rgb;
    if (any(isnan(historyColor)) || any(isinf(historyColor))) {
        historyColor = currentColor;
    }

    float3 neighborhoodMin;
    float3 neighborhoodMax;
    GetNeighborhoodBounds(pixelCoord, w, h, currentColor, neighborhoodMin, neighborhoodMax);
    historyColor = ClipHistoryToAABB(historyColor, neighborhoodMin, neighborhoodMax);

    float motionPixels = length(bestVelocity) * max(float(w), float(h));
    float motionFactor = saturate((motionPixels - 1.0f) / 16.0f);
    int prevDepthIndex = 1 - taaData.CurrentDepthBufferIndex;
    float prevDepth = SRV_DepthTextures[prevDepthIndex].SampleLevel(pointSS, prevUV, 0).r;
    float expectedPrevDepth = ReprojectCurrentDepthToPrevDepth(closestDepthUV, closestDepth);
    float depthDelta = abs(expectedPrevDepth - prevDepth);
    float disocclusionFactor = saturate((depthDelta - 0.0015f) * 250.0f);

    float responsiveWeight = max(taaData.BlendWeight, max(motionFactor, disocclusionFactor));
    float3 finalColor = lerp(historyColor, currentColor, responsiveWeight);

    psOut.result = float4(finalColor, 1.0f);
    return psOut;
}
