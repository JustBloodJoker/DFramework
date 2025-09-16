#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
#include <System/ClusteredLightningSystem.h>






void ClusteredLightningSystem::AfterConstruction() {
	auto device = m_pOwner->GetDevice();
	m_pClusterParamsBuffer = FD3DW::UploadBuffer<ClusterSystemClusterParams>::CreateConstantBuffer(device, 1);
	m_pClusterParamsPSBuffer = FD3DW::UploadBuffer<ClusterSystemClusterParamsPS>::CreateConstantBuffer(device, 1);
	m_pClusterViewParamsBuffer = FD3DW::UploadBuffer<ClusterSystemClusterViewParams>::CreateConstantBuffer(device, 1);
	m_pClustersStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<ClusterSystemCluster>(device, CLUSTERED_NUM_X_CLUSTERS * CLUSTERED_NUM_Y_CLUSTERS * CLUSTERED_NUM_Z_CLUSTERS, true, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

std::shared_ptr<FD3DW::ExecutionHandle> ClusteredLightningSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> sync) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COMPUTE, [this](ID3D12GraphicsCommandList* list) {
		ClusterSystemClusterParams par_1;
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


		ClusterSystemClusterViewParams par_2;
		par_2.LightCount = m_pOwner->GetLightsCount();
		dx::XMStoreFloat4x4(&par_2.ViewMatrix, m_pOwner->GetCurrentViewMatrix());

		ClusterSystemClusterParamsPS par_3;
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


		PSOManager::GetInstance()->GetPSOObject(PSOType::ClusteredShading_BuildGridPass)->Bind(list);

		list->SetComputeRootConstantBufferView(CLUSTERED_FIRST_PASS_CBV_CLUSTERS_PARAMS_POS_IN_ROOT_SIG, m_pClusterParamsBuffer->GetGPULocation(0));
		list->SetComputeRootUnorderedAccessView(CLUSTERED_FIRST_PASS_UAV_CLUSTERS_POS_IN_ROOT_SIG, m_pClustersStructuredBuffer->GetResource()->GetGPUVirtualAddress());

		list->Dispatch(CLUSTERED_NUM_X_CLUSTERS, CLUSTERED_NUM_Y_CLUSTERS, CLUSTERED_NUM_Z_CLUSTERS);
	});
	
	
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { sync });
}

std::shared_ptr<FD3DW::ExecutionHandle> ClusteredLightningSystem::AssignLightsToClusters(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> syncs) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COMPUTE, [this](ID3D12GraphicsCommandList* list) {
		PSOManager::GetInstance()->GetPSOObject(PSOType::ClusteredShading_LightsToClusteresPass)->Bind(list);

		list->SetComputeRootConstantBufferView(CLUSTERED_SECOND_PASS_CBV_VIEWP_POS_IN_ROOT_SIG, m_pClusterViewParamsBuffer->GetGPULocation(0));
		list->SetComputeRootUnorderedAccessView(CLUSTERED_SECOND_PASS_UAV_CLUSTERS_POS_IN_ROOT_SIG, m_pClustersStructuredBuffer->GetResource()->GetGPUVirtualAddress());
		
		list->SetComputeRootShaderResourceView(CLUSTERED_SECOND_PASS_SRV_LIGHTS_POS_IN_ROOT_SIG, m_pOwner->GetLightsBuffer()->GetResource()->GetGPUVirtualAddress());

		int numClusters = CLUSTERED_NUM_X_CLUSTERS * CLUSTERED_NUM_Y_CLUSTERS * CLUSTERED_NUM_Z_CLUSTERS;
		int numGroups = (numClusters + CLUSTERED_THREADS_PER_GROUP_X_2 - 1) / CLUSTERED_THREADS_PER_GROUP_X_2;
		list->Dispatch(numGroups, 1, 1);
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, syncs);
}

D3D12_GPU_VIRTUAL_ADDRESS ClusteredLightningSystem::GetClusteredStructuredBufferGPULocation() {
	return m_pClustersStructuredBuffer->GetResource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS ClusteredLightningSystem::GetClusteredConstantBufferGPULocation() {
	return m_pClusterParamsPSBuffer->GetGPULocation(0);
}


