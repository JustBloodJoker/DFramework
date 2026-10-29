#include "pch.h"

#include <System/RenderMeshesSystem.h>
#include <Renderer/OfflineRenderer.h>
#include <Renderer/GlobalTextureHeap.h>
#include <Renderer/PSOManager.h>
#include <Config/LightingConfig.h>
#include <Config/PipelineRegistry.h>
#include <Config/RendererLimits.h>
#include <Config/RenderFormats.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>
#include <Config/RendererResources.h>
#include <World/World.h>

void RenderMeshesSystem::AfterConstruction() {
	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[5] = {};
	argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	argumentDescs[0].ConstantBufferView.RootParameterIndex = CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG;
	argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	argumentDescs[1].ConstantBufferView.RootParameterIndex = CONSTANT_BUFFER_MATERIALS_POSITION_IN_ROOT_SIG;
	argumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
	argumentDescs[2].VertexBuffer.Slot = 0;
	argumentDescs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	argumentDescs[4].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
	commandSignatureDesc.pArgumentDescs = argumentDescs;
	commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
	commandSignatureDesc.ByteStride = sizeof(IndirectMeshRenderData);

	auto rootSignature = PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassDefaultConfig)->GetRootSignature();
	HRESULT_ASSERT(m_pOwner->GetDevice()->CreateCommandSignature(&commandSignatureDesc, rootSignature, IID_PPV_ARGS(m_pIndirectCommandSignature.ReleaseAndGetAddressOf())), "Incorrect creation of Indirect Command Signature");

	m_pIndirectExecuteCommandsBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<IndirectMeshRenderData>(m_pOwner->GetDevice(), 1u, true);
	m_pRTGeometryInstances = FD3DW::StructuredBuffer::CreateStructuredBuffer<NSRTGeometryInstance>(m_pOwner->GetDevice(), 1u, true);

}

void RenderMeshesSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::UpdateTLAS) {
		m_bNeedUpdateTLAS.store(true, std::memory_order_relaxed);
	}
	else if (type == NRenderSystemNotifyType::MeshActivationDeactivation) {
		m_bNeedUpdateMeshesActivationDeactivation.store(true, std::memory_order_relaxed);
		m_bNeedUpdateTLAS.store(true, std::memory_order_relaxed);

	}
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		if (m_bNeedUpdateMeshesActivationDeactivation.exchange(false, std::memory_order_acq_rel)) {

			m_vActiveMeshComponents.clear();
			m_vMeshRenderData.clear();
			m_vMeshSourceIds.clear();

			auto meshComponents = GetWorld()->GetAllComponentsOfType<MeshComponent>();
			for (const auto& cmp : meshComponents) {
				if (cmp->IsActive()) m_vActiveMeshComponents.push_back(cmp);
			}

			for (auto& cmp : m_vActiveMeshComponents) {
				m_vMeshRenderData.push_back(cmp->GetIndirectRenderData());
				m_vMeshSourceIds.push_back(static_cast<UINT>(cmp->GetCreationData().ID));
			}
			RefreshRTGeometryBindings(list);
			m_pIndirectExecuteCommandsBuffer->UploadData(m_pOwner->GetDevice(), list, m_vMeshRenderData.data(), int(m_vMeshRenderData.size()), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		}

		RenderComponentBeforeRenderInputData inData;
		inData.CameraPosition = m_pOwner->GetCurrentCameraPosition();
		inData.Projection = m_pOwner->GetCurrentProjectionMatrix();
		inData.View = m_pOwner->GetCurrentViewMatrix();

		for (auto& cmp : m_vActiveMeshComponents) {
			cmp->OnStartRenderTick(inData);
		}
		RefreshRTGeometryCoverageState();
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, {handle}, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::OnStartTLASCall(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList5>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList4* list) {
		if (!m_bNeedUpdateTLAS.exchange(false, std::memory_order_acq_rel)) return;

		std::vector<std::pair<FD3DW::AccelerationStructureBuffers, dx::XMMATRIX>> instances;
		for (const auto& cmp : m_vActiveMeshComponents) {
			if (cmp->HasCompleteBLAS()) {
				instances.push_back({ cmp->GetBLASBuffer() , cmp->GetWorldMatrix() });
			}
		}

		if (instances.empty())
		{
			m_xTLASBufferData = {};
		}
		else
		{
			FD3DW::UpdateTopLevelAS(m_pOwner->GetDXRDevice(), list, m_xTLASBufferData, instances);
		}
		RefreshRTGeometryCoverageState();
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, handle, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> RenderMeshesSystem::IndirectRender(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle, RenderMeshesSystemIndirectRenderData data) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, data](ID3D12GraphicsCommandList* list) {

		data.DSV->DepthWrite(list);
		list->ClearDepthStencilView(data.DSV_CPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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

			constexpr FLOAT clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
			for (UINT i = 0; i < gBuffersCount; ++i) {
				CD3DX12_CPU_DESCRIPTOR_HANDLE pdhHandle(data.RTV_CPU);
				pdhHandle.Offset(i, GetRTVDescriptorSize(m_pOwner->GetDevice()));
				list->ClearRenderTargetView(pdhHandle, clearColor, 0, nullptr);
			}
			list->OMSetRenderTargets(gBuffersCount, &FD3DW::keep(data.RTV_CPU), true, &FD3DW::keep(data.DSV_CPU));

			if (!m_vMeshRenderData.empty()) {
				PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassDefaultConfig)->Bind(list);
				ID3D12DescriptorHeap* heaps[] = { GlobalTextureHeap::GetInstance()->GetResult()->GetDescriptorPtr() };
				list->SetDescriptorHeaps(_countof(heaps), heaps);
				list->SetGraphicsRootDescriptorTable(TEXTURE_START_POSITION_IN_ROOT_SIG, GlobalTextureHeap::GetInstance()->GetResult()->GetGPUDescriptorHandle(0));

				auto dataSize = (UINT)m_vMeshRenderData.size();
				m_pIndirectExecuteCommandsBuffer->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				list->ExecuteIndirect(m_pIndirectCommandSignature.Get(), dataSize, m_pIndirectExecuteCommandsBuffer->GetResource(), 0, nullptr, 0);
			}

			for (auto& gbuffer : data.RTV) {
				gbuffer->EndDraw(list);
			}

			data.DSV->SRVPass(list);
		}
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, handle, true);
}

FD3DW::AccelerationStructureBuffers RenderMeshesSystem::GetTLAS() const {
	return m_xTLASBufferData;
}

void RenderMeshesSystem::RefreshRTGeometryBindings(ID3D12GraphicsCommandList* list) {
	std::vector<NSRTGeometryInstance> instances(m_vActiveMeshComponents.size());
	std::vector<NSRTGeometrySupportClass> support(m_vActiveMeshComponents.size(), NSRTGeometrySupportClass::OpaqueTriangles);
	
	NSRTGeometryAccounting accounting{};
	accounting.TotalInstances = static_cast<UINT>(m_vActiveMeshComponents.size());
	
	ID3D12Resource* sharedIndex = nullptr;
	ID3D12Resource* sharedVertex = nullptr;
	bool sharedCompatible = true;

	for (auto* cmp : m_vActiveMeshComponents) {
		if (!cmp || cmp->GetRTGeometryMaterialFacts().AlphaMode != NSRTGeometryAlphaMode::Mask) continue;
		
		const auto data = cmp->GetCreationData();
		if (!sharedIndex && !sharedVertex) {
			sharedIndex = data.IndexBuffer;
			sharedVertex = data.VertexBuffer;
		}
		else if (sharedIndex != data.IndexBuffer || sharedVertex != data.VertexBuffer) {
			sharedCompatible = false;
		}
	}
	accounting.SharedAlphaBuffersCompatible = sharedCompatible ? 1u : 0u;

	for (size_t index = 0u; index < m_vActiveMeshComponents.size(); ++index) {
		auto* cmp = m_vActiveMeshComponents[index];
		
		const auto data = cmp->GetCreationData();
		const bool textureIndexValid = data.MaterialCBufferData.LoadedTexture[FD3DW::TEXTURETYPE::BASE] >= 0 && data.MaterialCBufferData.LoadedTexture[FD3DW::TEXTURETYPE::BASE] < int(NSOfflineConfig::RendererLimits::TextureHeapPrecacheSize);
		const bool buffersCompatible = sharedCompatible && data.IndexBuffer && data.VertexBuffer && textureIndexValid;
		
		support[index] = cmp->GetRTGeometrySupportClass(buffersCompatible);
		
		auto& instance = instances[index];
		instance.IndexOffset = data.ObjectDescriptor.IndicesOffset;
		instance.VertexOffset = data.ObjectDescriptor.VerticesOffset;
		instance.VertexStride = data.VertexStructSize;
		instance.AlphaTextureIndex = textureIndexValid ? static_cast<UINT>(data.MaterialCBufferData.LoadedTexture[FD3DW::TEXTURETYPE::BASE]) : 0u;
		instance.AlphaCutoff = data.ObjectDescriptor.AlphaCutoff;
		instance.OpacityFactor = data.MaterialCBufferData.Diffuse.w;
		
		instance.Flags = (support[index] == NSRTGeometrySupportClass::AlphaTestedTriangles
			? static_cast<UINT>(NSRTGeometryInstanceFlags::AlphaTested) : 0u) |
			(cmp->IsDoubleSided() ? static_cast<UINT>(NSRTGeometryInstanceFlags::DoubleSided) : 0u);
		
		accounting.TriangleCount += data.ObjectDescriptor.IndicesCount / 3u;
		accounting.DoubleSidedInstances += cmp->IsDoubleSided() ? 1u : 0u;
		
		switch (support[index]) {
		case NSRTGeometrySupportClass::OpaqueTriangles: 
			++accounting.OpaqueInstances; 
			break;
		case NSRTGeometrySupportClass::AlphaTestedTriangles: 
			++accounting.AlphaTestedInstances; 
			break;
		default: 
			++accounting.UnsupportedAlphaOrOpacityInstances; 
			break;
		}

	}

	if (!instances.empty()) {
		m_pRTGeometryInstances->UploadData(m_pOwner->GetDevice(), list, instances.data(), static_cast<UINT>(instances.size()),D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	std::lock_guard<std::mutex> lock(m_xRTGeometryCoverageMutex);
	
	m_vRTGeometrySupportClasses = std::move(support);
	
	m_xRTGeometryAccounting = accounting;
	
	m_pRTAlphaIndexBuffer = sharedCompatible ? sharedIndex : nullptr;
	m_pRTAlphaVertexBuffer = sharedCompatible ? sharedVertex : nullptr;
}

void RenderMeshesSystem::RefreshRTGeometryCoverageState() {
	
	NSRTGeometryCoverageState state;
	NSRTGeometryAccounting accounting;
	
	std::vector<NSRTGeometrySupportClass> support;
	{
		std::lock_guard<std::mutex> lock(m_xRTGeometryCoverageMutex);
		accounting = m_xRTGeometryAccounting;
		support = m_vRTGeometrySupportClasses;
	}
	
	accounting.MissingBLASInstances = 0u;
	if (!m_vActiveMeshComponents.empty()) {
		state.Flags |= ToGeometryCoverageBits(NSRTGeometryCoverageFlags::OpaqueTriangles);
	}

	for (size_t index = 0u; index < m_vActiveMeshComponents.size(); ++index) {
		const auto& cmp = m_vActiveMeshComponents[index];
		++state.ActiveTriangleInstances;
		++state.RigidInstances;
		state.Flags |= ToGeometryCoverageBits(NSRTGeometryCoverageFlags::RigidTransforms);

		const auto supportClass = index < support.size() ? support[index] : NSRTGeometrySupportClass::UnsupportedAlphaOrOpacity;
		
		if (supportClass == NSRTGeometrySupportClass::AlphaTestedTriangles) {
			state.Flags |= ToGeometryCoverageBits(NSRTGeometryCoverageFlags::AlphaTestedTriangles);
		}
		else if (supportClass == NSRTGeometrySupportClass::UnsupportedAlphaOrOpacity) {
			++state.ConservativeAlphaOrOpacityInstances;
			state.Flags |= ToGeometryCoverageBits(NSRTGeometryCoverageFlags::ConservativeAlphaOrOpacityMask);
		}
		
		if (cmp->IsDoubleSided()) {
			state.Flags |= ToGeometryCoverageBits(NSRTGeometryCoverageFlags::TwoSidedThinTriangles);
		}
		
		if (!cmp->HasCompleteBLAS()) {
			++state.MissingBLASInstances;
			++accounting.MissingBLASInstances;
			state.Flags |= ToGeometryCoverageBits(NSRTGeometryCoverageFlags::MissingAccelerationStructure);
		}
	}

	state.TLASComplete = m_xTLASBufferData.pResult != nullptr ? 1u : 0u;
	if (state.TLASComplete == 0u) {
		state.Flags |= ToGeometryCoverageBits(NSRTGeometryCoverageFlags::MissingAccelerationStructure);
	}
	
	state.GroundTruthGeometryEligible = IsGroundTruthGeometryEligible(state) ? 1u : 0u;

	std::lock_guard<std::mutex> lock(m_xRTGeometryCoverageMutex);
	
	m_xRTGeometryCoverageState = state;
	m_xRTGeometryAccounting = accounting;
}

NSRTGeometryCoverageState RenderMeshesSystem::GetRTGeometryCoverageState() const {
	std::lock_guard<std::mutex> lock(m_xRTGeometryCoverageMutex);
	return m_xRTGeometryCoverageState;
}

NSRTGeometryAccounting RenderMeshesSystem::GetRTGeometryAccounting() const {
	std::lock_guard<std::mutex> lock(m_xRTGeometryCoverageMutex);
	return m_xRTGeometryAccounting;
}

FD3DW::FResource* RenderMeshesSystem::GetRTGeometryInstancesResource() const {
	return m_pRTGeometryInstances.get();
}

ID3D12Resource* RenderMeshesSystem::GetRTAlphaIndexBuffer() const {
	std::lock_guard<std::mutex> lock(m_xRTGeometryCoverageMutex);
	return m_pRTAlphaIndexBuffer;
}

ID3D12Resource* RenderMeshesSystem::GetRTAlphaVertexBuffer() const {
	std::lock_guard<std::mutex> lock(m_xRTGeometryCoverageMutex);
	return m_pRTAlphaVertexBuffer;
}
