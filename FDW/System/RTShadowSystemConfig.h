#pragma once 


#include <pch.h>
#include <WinWindow/Utils/Reflection/Reflection.h>
#include <D3DFramework/GraphicUtilites/BilateralFilter.h>

struct RTShadowSystemBuffer {
    dx::XMMATRIX PrevViewProj;
    dx::XMMATRIX CurrViewProj;
    float TemporalFeedbackMin = 0.02f;
    float TemporalFeedbackMax = 0.2f;
    float ReprojDistThreshold = 0.02f;
    float NormalThreshold = 0.9f;
    UINT FrameIndex;
    int padding[3];
};

struct RTShadowSystemConfig : public RTShadowSystemBuffer, public FD3DW::BilateralParams {

    REFLECT_STRUCT(RTShadowSystemConfig)
    BEGIN_REFLECT(RTShadowSystemConfig)
        REFLECT_PROPERTY(TemporalFeedbackMin)
        REFLECT_PROPERTY(TemporalFeedbackMax)
        REFLECT_PROPERTY(ReprojDistThreshold)
        REFLECT_PROPERTY(NormalThreshold)
        REFLECT_PROPERTY(SigmaS)
        REFLECT_PROPERTY(SigmaR)
        REFLECT_PROPERTY(KernelRadius)
    END_REFLECT(RTShadowSystemConfig)
};
