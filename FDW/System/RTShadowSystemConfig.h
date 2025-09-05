#pragma once 


#include <pch.h>
#include <D3DFramework/Utilites/Serializer/ReflectionImpl.h>
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

	BEGIN_FIELD_REGISTRATION(RTShadowSystemConfig)
		REGISTER_FIELD(TemporalFeedbackMin);
		REGISTER_FIELD(TemporalFeedbackMax);
		REGISTER_FIELD(ReprojDistThreshold);
		REGISTER_FIELD(NormalThreshold);
		REGISTER_FIELD(SigmaS);
		REGISTER_FIELD(SigmaR);
		REGISTER_FIELD(KernelRadius);
	END_FIELD_REGISTRATION();
};
