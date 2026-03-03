#pragma once

struct TAASystemData {
	int CurrentWriterIndex = 0;
	int CurrentReaderIndex = 0;
	int CurrentDepthBufferIndex = 0;
	float BlendWeight = 0.1f;
	dx::XMFLOAT2 CurrentJitterOffset;
	dx::XMFLOAT2 PrevJitterOffset;
};