#pragma once

#include <D3DFramework/GraphicUtilites/Materials.h>

struct MeshComponentMatricesData : FD3DW::MatricesConstantBufferStructureFrameWork {
	dx::XMFLOAT3 CameraPosition;
	float Padding = 0.0f;
};
