#include <System/RenderMeshesSystem.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <MainRenderer/PSOManager.h>
#include <World/World.h>

void RenderMeshesSystem::AfterConstruction() {
	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[6] = {};
	argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	argumentDescs[0].ConstantBufferView.RootParameterIndex = CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG;
	argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	argumentDescs[1].ConstantBufferView.RootParameterIndex = CONSTANT_BUFFER_MATERIALS_POSITION_IN_ROOT_SIG;
	argumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
	argumentDescs[2].ShaderResourceView.RootParameterIndex = ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG;
	argumentDescs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
	argumentDescs[3].VertexBuffer.Slot = 0;
	argumentDescs[4].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	argumentDescs[5].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
	commandSignatureDesc.pArgumentDescs = argumentDescs;
	commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
	commandSignatureDesc.ByteStride = sizeof(IndirectMeshRenderableData);

	auto rootSignature = PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassDefaultConfig)->GetRootSignature();
	HRESULT_ASSERT(m_pOwner->GetDevice()->CreateCommandSignature(&commandSignatureDesc, rootSignature, IID_PPV_ARGS(m_pIndirectCommandSignature.ReleaseAndGetAddressOf())), "Incorrect creation of Indirect Command Signature");

	m_pIndirectExecuteCommandsBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<IndirectMeshRenderableData>(m_pOwner->GetDevice(), 1u, true);


	m_pMeshesCulling = std::make_unique<MeshesCullingSubSystem>(m_pOwner->GetDevice());
}

void RenderMeshesSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::UpdateTLAS) {
		m_bNeedUpdateTLAS.store(true, std::memory_order_relaxed);
	}
	else if (type == NRenderSystemNotifyType::MeshActivationDeactivation) {
		m_bNeedUpdateMeshesActivationDeactivation.store(true, std::memory_order_relaxed);
	}
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		if (m_bNeedUpdateMeshesActivationDeactivation.exchange(false, std::memory_order_acq_rel)) {

			m_vActiveMeshComponents.clear();
			m_vMeshRenderData.clear();
			m_vMeshAABBInstanceData.clear();

			auto meshComponents = GetWorld()->GetAllComponentsOfType<MeshComponent>();
			for (const auto& cmp : meshComponents) {
				if (cmp->IsActive()) m_vActiveMeshComponents.push_back(cmp);
			}

			for (auto& cmp : m_vActiveMeshComponents) {
				auto [indirectExecute, AABBInstance] = cmp->GetIndirectRenderDataPair();
				m_vMeshRenderData.push_back(indirectExecute);
				
				AABBInstance.CommandIndex = (UINT)(m_vMeshRenderData.size() - 1);
				m_vMeshAABBInstanceData.push_back(AABBInstance);
			}
			m_pIndirectExecuteCommandsBuffer->UploadData(m_pOwner->GetDevice(), list, m_vMeshRenderData.data(), int(m_vMeshRenderData.size()), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		}

		RenderComponentBeforeRenderInputData inData;
		inData.CameraPosition = m_pOwner->GetCurrentCameraPosition();
		inData.CommandList = list;
		inData.Device = m_pOwner->GetDevice();
		inData.Projection = m_pOwner->GetCurrentProjectionMatrix();
		inData.View = m_pOwner->GetCurrentViewMatrix();
		auto timer = m_pOwner->GetTimer();

		inData.DT = timer->GetDeltaTime();
		inData.Time = timer->GetTime();

		for (auto& cmp : m_vActiveMeshComponents) {
			cmp->OnStartRenderTick(inData);
		}
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, {handle});
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::OnStartTLASCall(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList5>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList4* list) {
		if (!m_bNeedUpdateTLAS.exchange(false, std::memory_order_acq_rel)) return;

		std::vector<std::pair<FD3DW::AccelerationStructureBuffers, dx::XMMATRIX>> instances;
		for (const auto& cmp : m_vActiveMeshComponents) {
			instances.push_back({ cmp->GetBLASBuffer() , cmp->GetWorldMatrix() });
		}

		if (instances.empty()) 
		{
			m_xTLASBufferData = {};
		}
		else 
		{
			FD3DW::UpdateTopLevelAS(m_pOwner->GetDXRDevice(), list, m_xTLASBufferData, instances);
		}
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { handle });
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::UpdateHiZResource(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COMPUTE, [this](ID3D12GraphicsCommandList* list) {
		m_pMeshesCulling->UpdateHiZResource(m_pOwner->GetDepthResource(), m_pOwner->GetDevice(), list);
	});
	
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { handle });
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::PreDepthRender(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		for (auto& cmp : m_vActiveMeshComponents) {
			cmp->OnRenderPreDepthPass(list);
		}
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { handle });
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::IndirectRender(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		if (m_xMeshCullingType == MeshCullingType::GPU) {
			InputMeshesCullingProcessData data;
			data.CommandList = list;
			data.Device = m_pOwner->GetDevice();
			data.DepthResource = m_pOwner->GetDepthResource();
			data.InputCommandsBuffer = m_pIndirectExecuteCommandsBuffer.get();
			data.Instances = m_vMeshAABBInstanceData;
			data.CameraFrustum = m_pOwner->GetCameraFrustum();
			m_pMeshesCulling->ProcessGPUCulling(data);
		}

		PSOManager::GetInstance()->GetPSOObject(m_pOwner->IsEnabledPreDepth() ? PSOType::DefferedFirstPassWithPreDepth : PSOType::DefferedFirstPassDefaultConfig)->Bind(list);
		ID3D12DescriptorHeap* heaps[] = { GlobalTextureHeap::GetInstance()->GetResult()->GetDescriptorPtr() };
		list->SetDescriptorHeaps(_countof(heaps), heaps);
		list->SetGraphicsRootDescriptorTable(TEXTURE_START_POSITION_IN_ROOT_SIG, GlobalTextureHeap::GetInstance()->GetResult()->GetGPUDescriptorHandle(0));

		auto dataSize = (UINT)m_vMeshRenderData.size();
		auto instanceDataSize = (UINT)m_vMeshAABBInstanceData.size();
		if (m_xMeshCullingType == MeshCullingType::GPU) {
			auto cullingRes = m_pMeshesCulling->GetResultBuffer();
			cullingRes->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			list->ExecuteIndirect(m_pIndirectCommandSignature.Get(), dataSize, cullingRes->GetResource(), 0, cullingRes->GetResource(), m_pMeshesCulling->CountBufferOffset((UINT)instanceDataSize));
		}
		else {
			m_pIndirectExecuteCommandsBuffer->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			list->ExecuteIndirect(m_pIndirectCommandSignature.Get(), dataSize, m_pIndirectExecuteCommandsBuffer->GetResource(), 0, nullptr, 0);
		}
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { handle });
}