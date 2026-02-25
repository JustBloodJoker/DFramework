#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/Materials.h>

struct MeshComponentMatricesData : FD3DW::MatricesConstantBufferStructureFrameWork {
	dx::XMMATRIX PrevViewProjectionMatrix;
	int IsActiveAnimation = false;
	dx::XMFLOAT3 CameraPosition;
};