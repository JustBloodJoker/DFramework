#include <Lights/MainRenderer_LightsManager.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <MainRenderer/PSOManager.h>

void MainRenderer_LightsManager::AfterConstruction() {
	auto device = m_pOwner->GetDevice();
	m_pLightsHelperConstantBuffer = FD3DW::UploadBuffer<LightBuffer>::CreateConstantBuffer(device, 1);

	m_pLightsStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<LightStruct>(device, 1u, true);
	
	m_pClusterParamsBuffer = FD3DW::UploadBuffer<ClusterParams>::CreateConstantBuffer(device, 1);
	m_pClusterParamsPSBuffer = FD3DW::UploadBuffer<ClusterParamsPS>::CreateConstantBuffer(device, 1);
	m_pClusterViewParamsBuffer = FD3DW::UploadBuffer<ClusterViewParams>::CreateConstantBuffer(device, 1);
	m_pClustersStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<Cluster>(device, CLUSTERED_NUM_X_CLUSTERS * CLUSTERED_NUM_Y_CLUSTERS * CLUSTERED_NUM_Z_CLUSTERS, true, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	if (!m_vLights.empty()) m_bIsNeedUpdateLightsStructuredBuffer = true;
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
	UpdateLightsClustersData();
	UpdateLightsStructuredBuffer(list);
}

std::shared_ptr<FD3DW::ExecutionHandle> MainRenderer_LightsManager::ClusteredShadingPass(std::shared_ptr<FD3DW::ExecutionHandle> beforeRender) {
	auto recipe_buildGrid = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COMPUTE, [this](ID3D12GraphicsCommandList* list) {

		PSOManager::GetInstance()->GetPSOObject(PSOType::ClusteredShading_BuildGridPass)->Bind(list);

		list->SetComputeRootConstantBufferView(CLUSTERED_FIRST_PASS_CBV_CLUSTERS_PARAMS_POS_IN_ROOT_SIG, m_pClusterParamsBuffer->GetGPULocation(0));
		list->SetComputeRootUnorderedAccessView(CLUSTERED_FIRST_PASS_UAV_CLUSTERS_POS_IN_ROOT_SIG, m_pClustersStructuredBuffer->GetResource()->GetGPUVirtualAddress());

		list->Dispatch(CLUSTERED_NUM_X_CLUSTERS, CLUSTERED_NUM_Y_CLUSTERS, CLUSTERED_NUM_Z_CLUSTERS);
	});
	auto buildGridPassH = GlobalRenderThreadManager::GetInstance()->Submit(recipe_buildGrid, { beforeRender });
	
	auto recipe_setLightsToClusters = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COMPUTE, [this](ID3D12GraphicsCommandList* list) {
		PSOManager::GetInstance()->GetPSOObject(PSOType::ClusteredShading_LightsToClusteresPass)->Bind(list);

		list->SetComputeRootConstantBufferView(CLUSTERED_SECOND_PASS_CBV_VIEWP_POS_IN_ROOT_SIG, m_pClusterViewParamsBuffer->GetGPULocation(0));
		list->SetComputeRootUnorderedAccessView(CLUSTERED_SECOND_PASS_UAV_CLUSTERS_POS_IN_ROOT_SIG, m_pClustersStructuredBuffer->GetResource()->GetGPUVirtualAddress());
		list->SetComputeRootShaderResourceView(CLUSTERED_SECOND_PASS_SRV_LIGHTS_POS_IN_ROOT_SIG, m_pLightsStructuredBuffer->GetResource()->GetGPUVirtualAddress());

		int numClusters = CLUSTERED_NUM_X_CLUSTERS * CLUSTERED_NUM_Y_CLUSTERS * CLUSTERED_NUM_Z_CLUSTERS;
		int numGroups = (numClusters + CLUSTERED_THREADS_PER_GROUP_X_2 - 1) / CLUSTERED_THREADS_PER_GROUP_X_2;
		list->Dispatch(numGroups, 1, 1);
	});
	auto setLightsToClustersH = GlobalRenderThreadManager::GetInstance()->Submit(recipe_setLightsToClusters, { buildGridPassH });

	return setLightsToClustersH;
}

void MainRenderer_LightsManager::BindLightConstantBuffer(UINT cbSlot, UINT rootSRVSlot, UINT rootSRVClustersSlot, UINT cbClusterDataSlot, ID3D12GraphicsCommandList* list, bool IsCompute) {
	if (IsCompute) {
		list->SetComputeRootConstantBufferView(cbSlot, m_pLightsHelperConstantBuffer->GetGPULocation(0));
		list->SetComputeRootShaderResourceView(rootSRVSlot, m_pLightsStructuredBuffer->GetResource()->GetGPUVirtualAddress());
		list->SetComputeRootShaderResourceView(rootSRVClustersSlot, m_pClustersStructuredBuffer->GetResource()->GetGPUVirtualAddress());
		list->SetComputeRootConstantBufferView(cbClusterDataSlot, m_pClusterParamsPSBuffer->GetGPULocation(0));
	}
	else {
		list->SetGraphicsRootConstantBufferView(cbSlot, m_pLightsHelperConstantBuffer->GetGPULocation(0));
		list->SetGraphicsRootShaderResourceView(rootSRVSlot, m_pLightsStructuredBuffer->GetResource()->GetGPUVirtualAddress());
		list->SetGraphicsRootShaderResourceView(rootSRVClustersSlot, m_pClustersStructuredBuffer->GetResource()->GetGPUVirtualAddress());
		list->SetGraphicsRootConstantBufferView(cbClusterDataSlot, m_pClusterParamsPSBuffer->GetGPULocation(0));
	}
}

void MainRenderer_LightsManager::UpdateLightsConstantBuffer() {
	m_xLightBuffer.LightCount = int(m_vLights.size());
	m_xLightBuffer.CameraPos = m_pOwner->GetCurrentCameraPosition();
	m_xLightBuffer.IsShadowImpl = m_pOwner->CurrentShadowType() != ShadowType::None;

	m_pLightsHelperConstantBuffer->CpyData(0, m_xLightBuffer);
}

void MainRenderer_LightsManager::UpdateLightsStructuredBuffer(ID3D12GraphicsCommandList* list) {
	if (m_bIsNeedUpdateLightsStructuredBuffer) {
		auto device = m_pOwner->GetDevice();
		m_pLightsStructuredBuffer->UploadData(device, list, m_vLights.data(), UINT(m_vLights.size()));
		m_bIsNeedUpdateLightsStructuredBuffer = false;
	}
}

void MainRenderer_LightsManager::UpdateLightsClustersData() {
	ClusterParams par_1;
	par_1.GridSize0 = CLUSTERED_NUM_X_CLUSTERS;
	par_1.GridSize1 = CLUSTERED_NUM_Y_CLUSTERS;
	par_1.GridSize2 = CLUSTERED_NUM_Z_CLUSTERS;
	dx::XMMATRIX proj = m_pOwner->GetCurrentProjectionMatrix();
	dx::XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
	XMStoreFloat4x4(&par_1.InverseProjection, invProj);
	
	auto wndSet = m_pOwner->GetMainWNDSettings();
	par_1.ScreenWidth = wndSet.Width;
	par_1.ScreenHeight = wndSet.Height;

	auto frustum = m_pOwner->GetCameraFrustum();
	par_1.ZNear = frustum.GetZNear();
	par_1.ZFar = frustum.GetZFar();


	ClusterViewParams par_2;
	par_2.LightCount = int(m_vLights.size());
	dx::XMStoreFloat4x4(&par_2.ViewMatrix, m_pOwner->GetCurrentViewMatrix());

	ClusterParamsPS par_3;
	par_3.GridSize0 = par_1.GridSize0;
	par_3.GridSize1 = par_1.GridSize1;
	par_3.GridSize2 = par_1.GridSize2;
	par_3.ScreenWidth = par_1.ScreenWidth;
	par_3.ScreenHeight = par_1.ScreenHeight;
	par_3.ZNear = par_1.ZNear;
	par_3.ZFar = par_1.ZFar;
	dx::XMStoreFloat4x4(&par_3.ViewMatrix, m_pOwner->GetCurrentViewMatrix());
	m_pClusterParamsBuffer->CpyData(0, par_1);
	m_pClusterViewParamsBuffer->CpyData(0, par_2);
	m_pClusterParamsPSBuffer->CpyData(0, par_3);
}


