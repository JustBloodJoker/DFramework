#include <Lights/MainRenderer_ShadowsComponent.h>

void MainRenderer_ShadowsComponent::BindResultResource(ID3D12Device* device, FD3DW::SRV_UAVPacker* srv, size_t index) {
	srv->AddResource(GetResultResource()->GetResource(), GetSRVResultDimension(), index, device);
}

void MainRenderer_ShadowsComponent::BeforeRender(ID3D12GraphicsCommandList* list) {

}