#include <Lights/MainRenderer_ShadowsComponent.h>

void MainRenderer_ShadowsComponent::OnDrawResource(ID3D12GraphicsCommandList* list) {
	GetResultResource()->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void MainRenderer_ShadowsComponent::BindResultResource(ID3D12Device* device, FD3DW::SRV_UAVPacker* srv, size_t index) {
	srv->AddResource(GetResultResource()->GetResource(), GetSRVResultDimension(), index, device);
}

void MainRenderer_ShadowsComponent::BeforeRender(ID3D12GraphicsCommandList* list) {

}

ShadowType MainRenderer_ShadowsComponent::Type() {
	return ShadowType::None;
}
