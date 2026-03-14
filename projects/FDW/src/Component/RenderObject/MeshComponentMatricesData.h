#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/Materials.h>

struct MeshComponentMatricesData : FD3DW::MatricesConstantBufferStructureFrameWork {
	dx::XMMATRIX PrevWorld;
	dx::XMMATRIX JitteredProjection;
	dx::XMMATRIX PrevViewProjectionMatrix;
	int IsActiveAnimation = false;
	int SelectionState = 0;
	dx::XMFLOAT2 padd0 = { 0.0f, 0.0f };
	dx::XMFLOAT3 CameraPosition;
	float padd1 = 0.0f;
};
