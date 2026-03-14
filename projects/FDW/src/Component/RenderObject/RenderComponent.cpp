#include <Component/RenderObject/RenderComponent.h>
#include <World/World.h>


dx::XMMATRIX RenderComponent::GetWorldMatrix() const {
	return m_xWorldMatrix;
}

void RenderComponent::RenderInitDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) {}
