#include <Entity/RenderObject/SkyboxComponent.h>
#include <World/World.h>
#include "MeshComponent.h"

SkyboxComponent::SkyboxComponent(std::string path) {
	m_sPathToTexture = path;
	m_sName = m_sPathToTexture;
}

void SkyboxComponent::OnStartRenderTick(const RenderComponentBeforeRenderInputData& data) {
	m_xMatricesData.Projection = dx::XMMatrixTranspose(data.Projection);

	dx::XMMATRIX viewNoTranslation = data.View;
	viewNoTranslation.r[3] = dx::XMVectorSet(0, 0, 0, 1);
	m_xMatricesData.View = dx::XMMatrixTranspose(viewNoTranslation);

	m_xMatricesData.World = dx::XMMatrixTranspose(dx::XMMatrixIdentity());

}

std::shared_ptr<FD3DW::ExecutionHandle> SkyboxComponent::RenderInit(ID3D12Device* device, std::shared_ptr<FD3DW::ExecutionHandle> sync) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, device](ID3D12GraphicsCommandList* list) {
		m_pCube = std::make_unique<FD3DW::Cube>(device, list);
		
		auto cbvsrvuavsize = GetCBV_SRV_UAVDescriptorSize(device);

		m_pMaterial = std::make_unique<FD3DW::Material>();
		m_pMaterial->SetTexture(m_sPathToTexture, FD3DW::TextureType::BASE, device, list);

		m_pSRVPack = FD3DW::SRV_UAVPacker::CreatePack(cbvsrvuavsize, 1, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
		m_pSRVPack->AddResource(m_pMaterial->GetResourceTexture(FD3DW::TextureType::BASE), D3D12_SRV_DIMENSION_TEXTURECUBE, 0, device);
	});
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, {sync});
}

void SkyboxComponent::OnEndRenderTick(ID3D12GraphicsCommandList* list) {}

void SkyboxComponent::Init() {
	RenderComponent::Init();

	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::SkyboxActivationDeactivation);
}

void SkyboxComponent::Destroy() {
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::SkyboxActivationDeactivation);
}

void SkyboxComponent::Activate(bool a) {
	SkyboxComponent::Activate(a);
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::SkyboxActivationDeactivation);
}

std::string SkyboxComponent::PathToTexture() const { return m_sPathToTexture; }

FD3DW::SRV_UAVPacker* SkyboxComponent::SRVPack() const { return m_pSRVPack.get(); }

FD3DW::Material* SkyboxComponent::Material() const { return m_pMaterial.get(); }

FD3DW::Cube* SkyboxComponent::Cube() const { return m_pCube.get(); }

FD3DW::MatricesConstantBufferStructureFrameWork SkyboxComponent::MatricesData() { return m_xMatricesData; }
