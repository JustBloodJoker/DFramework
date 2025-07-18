#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/Materials.h>

struct MeshMatricesStructure : FD3DW::MatricesConstantBufferStructureFrameWork {
	bool IsActiveAnimation = false;
	dx::XMFLOAT3 CameraPosition;
};
