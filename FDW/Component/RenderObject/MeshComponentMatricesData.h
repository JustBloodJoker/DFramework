#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/Materials.h>

struct MeshComponentMatricesData : FD3DW::MatricesConstantBufferStructureFrameWork {
	int IsActiveAnimation = false;
	dx::XMFLOAT3 CameraPosition;
};