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

	LoadDataToCameraBuffer(data.CameraFrustum, sizeInstances, data.DepthResource);
	LoadDataToInstancesBuffer(data.Device, data.CommandList, data.Instances);
	RecreateOutputCommandBuffer(data.Device, data.CommandList, sizeInstances);

	CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_pOutputCommandsBuffer->GetResource());
	data.CommandList->ResourceBarrier(1, &uavBarrier);

	PSOManager::GetInstance()->GetPSOObject(PSOType::ObjectsCullingDefaultConfig)->Bind(data.CommandList);

	ID3D12DescriptorHeap* heaps[] = { m_pPack->GetResult()->GetDescriptorPtr() };
	data.CommandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);
	data.CommandList->SetComputeRootConstantBufferView(CULLING_CONSTANT_BUFFER_VIEW_POS_IN_ROOT_SIG, m_pCullingCameraBuffer->GetGPULocation(0));
	data.CommandList->SetComputeRootShaderResourceView(CULLING_SRV_BUFFER_INSTANCES_DATA_POS_IN_ROOT_SIG, m_pInstancesDataBuffer->GetResource()->GetGPUVirtualAddress());
	data.CommandList->SetComputeRootShaderResourceView(CULLING_SRV_BUFFER_INPUT_COMMANDS_POS_IN_ROOT_SIG, data.InputCommandsBuffer->GetResource()->GetGPUVirtualAddress());
	data.CommandList->SetComputeRootDescriptorTable(CULLING_UAV_BUFFER_OUTPUT_COMMANDS_POS_IN_ROOT_SIG, m_pPack->GetResult()->GetGPUDescriptorHandle(0));

	UINT threadGroups = (sizeInstances + CULLING_THREAD_GROUP_SIZE - 1) / CULLING_THREAD_GROUP_SIZE;
	data.CommandList->Dispatch(threadGroups, 1, 1);
}

FD3DW::StructuredBuffer* ObjectCulling::GetResultBuffer() {
	return m_pOutputCommandsBuffer.get();
}

void ObjectCulling::Init(ID3D12Device* device) {
	m_pInstancesDataBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<InstanceData>(device, 1, true);
	m_pCullingCameraBuffer = FD3DW::UploadBuffer<CullingCameraStructure>::CreateConstantBuffer(device, 1);
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

	if (!m_pPack) {
		auto descSize = GetCBV_SRV_UAVDescriptorSize(device);
		m_pPack = std::make_unique<FD3DW::SRV_UAVPacker>(descSize, 1u, 0u, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
	}

	m_pPack->AddResource(GetDefaultDescriptor(count), 0, device);
}

void ObjectCulling::LoadDataToInstancesBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* list, std::vector<InstanceData> data) {
	m_pInstancesDataBuffer->UploadData(device, list, data.data(), (UINT)data.size(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void ObjectCulling::LoadDataToCameraBuffer(CameraFrustum frustum, UINT instancesCount, FD3DW::DepthStencilView* resource) {
	CullingCameraStructure ccs;
	ccs.CameraPlanes = frustum.GetPlanes();
	ccs.InstancesCount = instancesCount;
	ccs.MipLevels = resource ? resource->GetResource()->GetDesc().MipLevels : -1;
	m_pCullingCameraBuffer->CpyData(0, ccs);
}

static inline UINT AlignForUavCounter(UINT bufferSize)
{
	const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
	return (bufferSize + (alignment - 1)) & ~(alignment - 1);
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
	return AlignForUavCounter(count * sizeof(IndirectMeshRenderableData));
}
