#include "pch.h"

#include <Entity/RenderObject/SimpleMeshTemplates.h>
#include <World/World.h>

void TSimplePlane::AfterCreation() {
	m_sName = "SimplePlane";
}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> TSimplePlane::CreateSimpleObject(
	ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::VertexFrameWork>>(
		device, list, GenerateSimplePlaneScene);
}

void TSimpleCube::AfterCreation() {
	m_sName = "SimpleCube";
}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> TSimpleCube::CreateSimpleObject(
	ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::VertexFrameWork>>(
		device, list, GenerateSimpleCubeScene);
}

void TSimpleCone::AfterCreation() {
	m_sName = "SimpleCone";
}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> TSimpleCone::CreateSimpleObject(
	ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::VertexFrameWork>>(
		device, list, GenerateSimpleConeScene);
}

void TSimpleSphere::AfterCreation() {
	m_sName = "SimpleSphere";
}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> TSimpleSphere::CreateSimpleObject(
	ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::VertexFrameWork>>(
		device, list, GenerateSimpleSphereScene);
}

void TSimpleCylinder::AfterCreation() {
	m_sName = "SimpleCylinder";
}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> TSimpleCylinder::CreateSimpleObject(
	ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::VertexFrameWork>>(
		device, list, GenerateSimpleCylinderScene);
}
