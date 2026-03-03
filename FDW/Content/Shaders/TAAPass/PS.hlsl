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

    float2 offsets[4] = { float2(0, -texelSize.y), float2(0, texelSize.y), float2(-texelSize.x, 0), float2(texelSize.x, 0) };

    float  closestDepth = SRV_DepthTextures[taaData.CurrentDepthBufferIndex].SampleLevel(pointSS, uv, 0).r;
    float2 bestVelocity = MotionBuffer.SampleLevel(pointSS, uv, 0).xy;

    [unroll]
    for(int i = 0; i < 4; i++) {
        float2 neighborUV = uv + offsets[i];
        float neighborDepth = SRV_DepthTextures[taaData.CurrentDepthBufferIndex].SampleLevel(pointSS, neighborUV, 0).r;
        
        if (neighborDepth < closestDepth) { 
            closestDepth = neighborDepth;
            bestVelocity = MotionBuffer.SampleLevel(pointSS, neighborUV, 0).xy;
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

    float3 cC = currentColor;
    float3 cT = RenderOutput.SampleLevel(pointSS, uv + float2(0, -texelSize.y), 0).rgb;
    float3 cB = RenderOutput.SampleLevel(pointSS, uv + float2(0, texelSize.y), 0).rgb;
    float3 cL = RenderOutput.SampleLevel(pointSS, uv + float2(-texelSize.x, 0), 0).rgb;
    float3 cR = RenderOutput.SampleLevel(pointSS, uv + float2(texelSize.x, 0), 0).rgb;

    float3 minColor = min(cC, min(cT, min(cB, min(cL, cR))));
    float3 maxColor = max(cC, max(cT, max(cB, max(cL, cR))));

    float3 clampedHistory = clamp(historyColor, minColor, maxColor);

    float isMoving = saturate(length(bestVelocity) * 1000.0f);

    historyColor = lerp(historyColor, clampedHistory, isMoving);

    float3 finalColor = lerp(historyColor, currentColor, taaData.BlendWeight);

    psOut.result = float4(finalColor, 1.0f);
    return psOut;
}