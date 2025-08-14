#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/Materials.h>

struct MeshMatricesStructure : FD3DW::MatricesConstantBufferStructureFrameWork {
	int StartIndexInBoneBuffer = -1;
	dx::XMFLOAT3 CameraPosition;
};
