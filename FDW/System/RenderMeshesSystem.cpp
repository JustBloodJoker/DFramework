#include <System/RenderMeshesSystem.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <MainRenderer/PSOManager.h>
#include <MainRenderer/GlobalConfig.h>
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
	commandSignatureDesc.ByteStride = sizeof(IndirectMeshRenderData);

	auto rootSignature = PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassDefaultConfig)->GetRootSignature();
	HRESULT_ASSERT(m_pOwner->GetDevice()->CreateCommandSignature(&commandSignatureDesc, rootSignature, IID_PPV_ARGS(m_pIndirectCommandSignature.ReleaseAndGetAddressOf())), "Incorrect creation of Indirect Command Signature");

	m_pIndirectExecuteCommandsBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<IndirectMeshRenderData>(m_pOwner->GetDevice(), 1u, true);


	m_pMeshesCulling = std::make_unique<MeshesCullingSubSystem>(m_pOwner->GetDevice());
}

void RenderMeshesSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::UpdateTLAS) {
		m_bNeedUpdateTLAS.store(true, std::memory_order_relaxed);
	}
	else if (type == NRenderSystemNotifyType::UpdateBLAS) {
		m_bNeedUpdateBLAS.store(true, std::memory_order_relaxed);
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

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::OnStartBLASCall(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList5>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList4* list) {
		if (!m_bNeedUpdateBLAS.exchange(false, std::memory_order_acq_rel)) return;

		for (const auto& cmp : m_vActiveMeshComponents) {
			cmp->UpdateBLASDXR(m_pOwner->GetDXRDevice(), list);
		}
		m_bNeedUpdateTLAS.store(true, std::memory_order_relaxed);
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, handle);
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::OnStartTLASCall(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle) {
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

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, handle);
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::UpdateHiZResource(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle, RenderMeshesSystemHiZUpdateRenderData data) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COMPUTE, [this, data](ID3D12GraphicsCommandList* list) {
		m_pMeshesCulling->UpdateHiZResource(data.DSV, m_pOwner->GetDevice(), list);
	});
	
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, handle);
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::PreDepthRender(std::shared_ptr<FD3DW::ExecutionHandle> handle, RenderMeshesSystemPreDepthRenderData data) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, data](ID3D12GraphicsCommandList* list) {
		list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		data.DSV->DepthWrite(list);
		list->ClearDepthStencilView(data.DSV_CPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		PSOManager::GetInstance()->GetPSOObject(PSOType::PreDepthDefaultConfig)->Bind(list);

		list->RSSetScissorRects(1, &data.Rect);
		list->RSSetViewports(1, &data.Viewport);
		list->OMSetRenderTargets(0, nullptr, false, &FD3DW::keep(data.DSV_CPU));

		for (auto& cmp : m_vActiveMeshComponents) {
			cmp->OnRenderPreDepthPass(list);
		}
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { handle });
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::IndirectRender(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle, RenderMeshesSystemIndirectRenderData data) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, data](ID3D12GraphicsCommandList* list) {
		
		if (!m_pOwner->IsEnabledPreDepth())
		{
			data.DSV->DepthWrite(list);
			list->ClearDepthStencilView(data.DSV_CPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		}
		else
		{
			data.DSV->DepthRead(list);
		}

		///////////////////////
		//	DEFERRED FIRST PASS
		{
			list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			auto gBuffersCount = GetGBuffersNum();

			for (auto& gbuffer : data.RTV) {
				gbuffer->StartDraw(list);
			}

			list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			list->RSSetScissorRects(1, &data.Rect);
			list->RSSetViewports(1, &data.Viewport);

			for (UINT i = 0; i < gBuffersCount; ++i) {
				CD3DX12_CPU_DESCRIPTOR_HANDLE pdhHandle(data.RTV_CPU);
				pdhHandle.Offset(i, GetRTVDescriptorSize(m_pOwner->GetDevice()));
				list->ClearRenderTargetView(pdhHandle, m_pOwner->GetClearColor(), 0, nullptr);
			}
			list->OMSetRenderTargets(gBuffersCount, &FD3DW::keep(data.RTV_CPU), true, &FD3DW::keep(data.DSV_CPU));

			if (!m_vMeshAABBInstanceData.empty()) {
				if (m_xMeshCullingType == MeshCullingType::GPU) {
					InputMeshesCullingProcessData cullData;
					cullData.CommandList = list;
					cullData.Device = m_pOwner->GetDevice();
					cullData.DepthResource = data.DSV;
					cullData.InputCommandsBuffer = m_pIndirectExecuteCommandsBuffer.get();
					cullData.Instances = m_vMeshAABBInstanceData;
					cullData.CameraFrustum = m_pOwner->GetCameraFrustum();
					m_pMeshesCulling->ProcessGPUCulling(cullData);
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
			}
			
			for (auto& gbuffer : data.RTV) {
				gbuffer->EndDraw(list);
			}
		}
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, handle);
}

FD3DW::AccelerationStructureBuffers RenderMeshesSystem::GetTLAS() const {
	return m_xTLASBufferData;
}

MeshCullingType RenderMeshesSystem::GetCullingType() const {
	return m_xMeshCullingType;
}

void RenderMeshesSystem::SetCullingType(MeshCullingType type) {
	m_xMeshCullingType = type;
}
