#include <RenderableObjects/ObjectsCulling/ObjectCulling.h>
#include <RenderableObjects/IndirectMeshRenderableData.h>
#include <MainRenderer/GlobalConfig.h>
#include <MainRenderer/PSOManager.h>

ObjectCulling::ObjectCulling(ID3D12Device* device) {
	Init(device);
}

bool ObjectCulling::CheckFrustumCulling(CameraFrustum fr, const InstanceData& data) {
	return fr.IsAABBInsideFrustum(data.MinP, data.MaxP);
}

void ObjectCulling::ProcessGPUCulling(const InputObjectCullingProcessData& data) {
	auto sizeInstances = (UINT)data.Instances.size();

	LoadDataToCameraBuffer(data.CameraFrustum, sizeInstances);
	LoadDataToInstancesBuffer(data.Device, data.CommandList, data.Instances);
	RecreateOutputCommandBuffer(data.Device, data.CommandList, sizeInstances);

	CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_pOutputCommandsBuffer->GetResource());
	data.CommandList->ResourceBarrier(1, &uavBarrier);

	m_pHiZResource->ResourceBarrierChange(data.CommandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	PSOManager::GetInstance()->GetPSOObject(PSOType::ObjectsCullingDefaultConfig)->Bind(data.CommandList);

	ID3D12DescriptorHeap* heaps[] = { m_pPack->GetResult()->GetDescriptorPtr() };
	data.CommandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);
	data.CommandList->SetComputeRootConstantBufferView(OBJECT_GPU_CULLING_CONSTANT_BUFFER_VIEW_POS_IN_ROOT_SIG, m_pCullingCameraBuffer->GetGPULocation(0));
	data.CommandList->SetComputeRootShaderResourceView(OBJECT_GPU_CULLING_SRV_BUFFER_INSTANCES_DATA_POS_IN_ROOT_SIG, m_pInstancesDataBuffer->GetResource()->GetGPUVirtualAddress());
	data.CommandList->SetComputeRootShaderResourceView(OBJECT_GPU_CULLING_SRV_BUFFER_INPUT_COMMANDS_POS_IN_ROOT_SIG, data.InputCommandsBuffer->GetResource()->GetGPUVirtualAddress());
	data.CommandList->SetComputeRootDescriptorTable(OBJECT_GPU_CULLING_HIZ_RESOURCE_POS_IN_ROOT_SIG, m_pPack->GetResult()->GetGPUDescriptorHandle(OBJECT_CULLING_OUTPUT_SRV_HIZ_POS_IN_PACK));
	data.CommandList->SetComputeRootDescriptorTable(OBJECT_GPU_CULLING_UAV_BUFFER_OUTPUT_COMMANDS_POS_IN_ROOT_SIG, m_pPack->GetResult()->GetGPUDescriptorHandle(OBJECT_CULLING_OUTPUT_COMMANDS_POS_IN_PACK));

	UINT threadGroups = (sizeInstances + OBJECT_GPU_CULLING_THREAD_GROUP_SIZE - 1) / OBJECT_GPU_CULLING_THREAD_GROUP_SIZE;
	data.CommandList->Dispatch(threadGroups, 1, 1);

}

void ObjectCulling::UpdateHiZResource(FD3DW::DepthStencilView* mainDSV, ID3D12Device* device, ID3D12GraphicsCommandList* list)
{
	if (m_pLastDepthBufferResource != mainDSV) {
		m_pLastDepthBufferResource = mainDSV;
		
		if (m_pLastDepthBufferResource) {
			m_pPack->AddResource(m_pLastDepthBufferResource, OBJECT_CULLING_INPUT_DSV_POS_IN_PACK, device);
			m_bIsCanDoHiZOcclusion = true;
		} 
		else {
			m_pPack->AddNullResource(OBJECT_CULLING_INPUT_DSV_POS_IN_PACK, device);
			m_bIsCanDoHiZOcclusion = false;
			return;
		}
	}

	PSOManager::GetInstance()->GetPSOObject(PSOType::CopyDepthToHIZ)->Bind(list);

	m_pHiZResource->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	
	ID3D12DescriptorHeap* heaps[] = { m_pPack->GetResult()->GetDescriptorPtr() };
	list->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);
	list->SetComputeRootDescriptorTable(OBJECT_CULLING_HIZ_COPY_DEPTH_INPUT_POS_IN_ROOT_SIG, m_pPack->GetResult()->GetGPUDescriptorHandle(OBJECT_CULLING_INPUT_DSV_POS_IN_PACK));
	list->SetComputeRootDescriptorTable(OBJECT_CULLING_HIZ_COPY_DEPTH_OUTPUT_POS_IN_ROOT_SIG, m_pPack->GetResult()->GetGPUDescriptorHandle(OBJECT_CULLING_OUTPUT_UAV_HIZ_POS_IN_PACK));

	const auto& hizDesc = m_pHiZResource->GetResource()->GetDesc();
	const auto& dsvDesc = m_pLastDepthBufferResource->GetResource()->GetDesc();
	auto width = std::min(dsvDesc.Width, hizDesc.Width);
	auto height = std::min(dsvDesc.Height, hizDesc.Height);
	
	auto dispatchX = (width + OBJECT_CULLING_HIZ_COPY_DEPTH_THREAD_X_GROUP - 1) / OBJECT_CULLING_HIZ_COPY_DEPTH_THREAD_X_GROUP;
	auto dispatchY = (height + OBJECT_CULLING_HIZ_COPY_DEPTH_THREAD_Y_GROUP - 1) / OBJECT_CULLING_HIZ_COPY_DEPTH_THREAD_Y_GROUP;

	list->Dispatch(UINT(dispatchX), UINT(dispatchY), 1);

	m_pHiZResource->GenerateMips(device, list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

FD3DW::StructuredBuffer* ObjectCulling::GetResultBuffer() {
	return m_pOutputCommandsBuffer.get();
}

void ObjectCulling::Init(ID3D12Device* device) {
	m_pInstancesDataBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<InstanceData>(device, 1, true);
	m_pCullingCameraBuffer = FD3DW::UploadBuffer<CullingCameraStructure>::CreateConstantBuffer(device, 1);

	auto descSize = GetCBV_SRV_UAVDescriptorSize(device);
	m_pPack = std::make_unique<FD3DW::SRV_UAVPacker>(descSize, 4u, 0u, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);

	m_pHiZResource = FD3DW::FResource::CreateAnonimTexture(device, 1u, DXGI_FORMAT_R32_FLOAT, 1024, 1024, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 4u);

	m_pPack->AddResource(m_pHiZResource->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D,OBJECT_CULLING_OUTPUT_SRV_HIZ_POS_IN_PACK, device);
	
	FD3DW::UAVResourceDesc hiZDesc;
	hiZDesc.Format = m_pHiZResource->GetResource()->GetDesc().Format;
	hiZDesc.MipSlice = 0u;
	hiZDesc.PlaneSlice = 0u;
	hiZDesc.Resource = m_pHiZResource->GetResource();
	hiZDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	m_pPack->AddResource(hiZDesc, OBJECT_CULLING_OUTPUT_UAV_HIZ_POS_IN_PACK, device);
}

void ObjectCulling::RecreateOutputCommandBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* list, UINT count) {
	if (m_pOutputCommandsBuffer && FD3DW::StructuredBuffer::CalculateElementsCount(CountBufferOffset(count), (UINT)sizeof(IndirectMeshRenderableData)) == m_pOutputCommandsBuffer->GetCapacity()) {
		m_pOutputCommandsBuffer->Clear(device, list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		return;
	}

	if (!m_pOutputCommandsBuffer) {
		m_pOutputCommandsBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer(device, CountBufferOffset(count)+(UINT)sizeof(UINT), (UINT)sizeof(IndirectMeshRenderableData), true, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}
	else {
		m_pOutputCommandsBuffer->ReserveSize(device, CountBufferOffset(count) + (UINT)sizeof(UINT), (UINT)sizeof(IndirectMeshRenderableData));
	}

	m_pPack->AddResource(GetDefaultDescriptor(count), OBJECT_CULLING_OUTPUT_COMMANDS_POS_IN_PACK, device);
}

void ObjectCulling::LoadDataToInstancesBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* list, std::vector<InstanceData> data) {
	m_pInstancesDataBuffer->UploadData(device, list, data.data(), (UINT)data.size(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void ObjectCulling::LoadDataToCameraBuffer(CameraFrustum frustum, UINT instancesCount) {
	CullingCameraStructure ccs;
	ccs.CameraPlanes = frustum.GetPlanes();
	ccs.ViewProjection = frustum.GetViewProjection();
	ccs.InstancesCount = instancesCount;
	if (m_bIsCanDoHiZOcclusion && m_pHiZResource) {
		auto desc = m_pHiZResource->GetResource()->GetDesc();
		ccs.HiZWidth = desc.Width;
		ccs.HiZHeight = desc.Height;
		ccs.MipLevels = desc.MipLevels;
	}
	else
	{
		ccs.HiZHeight = -1;
		ccs.HiZWidth = -1;
		ccs.MipLevels = -1;
	}
	m_pCullingCameraBuffer->CpyData(0, ccs);
}


FD3DW::UAVResourceDesc ObjectCulling::GetDefaultDescriptor(UINT commandsNum) {
	FD3DW::UAVResourceDesc desc;

	desc.Resource = m_pOutputCommandsBuffer->GetResource();
	desc.CounterOffsetBytes = CountBufferOffset(commandsNum);
	desc.CounterResource = m_pOutputCommandsBuffer->GetResource();
	desc.FirstElement = 0;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.NumElements = commandsNum;
	desc.Stride = sizeof(IndirectMeshRenderableData);
	
	return desc;
}

UINT ObjectCulling::CountBufferOffset(UINT count) {
	return FD3DW::AlignForUavCounter(count * sizeof(IndirectMeshRenderableData));
}
