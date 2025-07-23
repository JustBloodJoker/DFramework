#include <Lights/MainRenderer_LightsManager.h>
#include <MainRenderer/MainRenderer.h>

MainRenderer_LightsManager::MainRenderer_LightsManager(MainRenderer* owner) : MainRendererComponent(owner) {}

void MainRenderer_LightsManager::AfterConstruction() {
	auto device = m_pOwner->GetDevice();
	m_pLightsHelperConstantBuffer = FD3DW::UploadBuffer<LightBuffer>::CreateConstantBuffer(device, 1);

	m_pLightsStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<LightStruct>(device, 1u, true);
}

void MainRenderer_LightsManager::InitLTC(ID3D12GraphicsCommandList* list, FD3DW::SRVPacker* srvPack) {
	auto device = m_pOwner->GetDevice();
	auto size = GetCBV_SRV_UAVDescriptorSize(device);

	auto LTCMat = FD3DW::FResource::CreateTextureFromPath(LIGHTS_LTC_TEXTURES_PATH_MAT, device, list);
	srvPack->AddResource(LTCMat->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, LIGHTS_LTC_MAT_LOCATION_IN_HEAP, device);

	auto LTCAmp = FD3DW::FResource::CreateTextureFromPath(LIGHTS_LTC_TEXTURES_PATH_AMP, device, list);
	srvPack->AddResource(LTCAmp->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, LIGHTS_LTC_AMP_LOCATION_IN_HEAP, device);

	m_vLCTResources.push_back(LTCMat);
	m_vLCTResources.push_back(LTCAmp);
}

void MainRenderer_LightsManager::AddLight(LightStruct light) {
	m_vLights.push_back(light);
	m_bIsNeedUpdateLightsStructuredBuffer = true;
}

void MainRenderer_LightsManager::DeleteLight(int idx) {
	if (idx >= 0 && idx < int(m_vLights.size())) {
		m_vLights.erase(m_vLights.begin() + idx);
		m_bIsNeedUpdateLightsStructuredBuffer = true;
	}
}

const LightStruct& MainRenderer_LightsManager::GetLight(int idx) {
	static const LightStruct s_sEmpty;
	if (idx >= 0 && idx < int(m_vLights.size())) {
		return m_vLights[idx];
	}
	return s_sEmpty;
}

void MainRenderer_LightsManager::SetLightData(LightStruct newData, int idx) {
	if (idx >= 0 && idx < int(m_vLights.size())) {
		m_vLights[idx] = newData;
		m_bIsNeedUpdateLightsStructuredBuffer = true;
	}
}

int MainRenderer_LightsManager::GetLightsCount() {
	return int(m_vLights.size());
}

void MainRenderer_LightsManager::BeforeRender(ID3D12GraphicsCommandList* list) {
	UpdateLightsConstantBuffer();	
	UpdateLightsStructuredBuffer(list);
}

void MainRenderer_LightsManager::BindLightConstantBuffer(UINT cbSlot, UINT rootSRVSlot, ID3D12GraphicsCommandList* list) {
	list->SetGraphicsRootConstantBufferView(cbSlot, m_pLightsHelperConstantBuffer->GetGPULocation(0));
	list->SetGraphicsRootShaderResourceView(rootSRVSlot, m_pLightsStructuredBuffer->GetResource()->GetGPUVirtualAddress());
}

void MainRenderer_LightsManager::UpdateLightsConstantBuffer() {
	m_xLightBuffer.LightCount = int(m_vLights.size());
	m_xLightBuffer.CameraPos = m_pOwner->GetCurrentCameraPosition();

	m_pLightsHelperConstantBuffer->CpyData(0, m_xLightBuffer);
}

void MainRenderer_LightsManager::UpdateLightsStructuredBuffer(ID3D12GraphicsCommandList* list) {
	if (m_bIsNeedUpdateLightsStructuredBuffer) {
		auto device = m_pOwner->GetDevice();
		m_pLightsStructuredBuffer->UploadData(device, list, m_vLights.data(), UINT(m_vLights.size()));
		m_bIsNeedUpdateLightsStructuredBuffer = false;
	}
}

