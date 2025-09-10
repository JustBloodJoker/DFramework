#include <Component/RenderObject/MeshComponent.h>
#include <World/World.h>
#include <D3DFramework/Objects/RTObjectHelper.h>

MeshComponent::MeshComponent() {
    m_sName = "MeshComponent";
}

void MeshComponent::SetParentPosition(dx::XMFLOAT3 parentPosition) {
    m_xParentPosition = parentPosition;
    UpdateWorldMatrix();
}

void MeshComponent::SetParentRotation(dx::XMFLOAT3 parentRotation) {
    m_xParentRotation = parentRotation;
    UpdateWorldMatrix();
}

void MeshComponent::SetParentScale(dx::XMFLOAT3 parentScale) { 
    m_xParentScaling = parentScale;
    UpdateWorldMatrix();
}

bool MeshComponent::IsIgnoreParentRotation() {
    return m_bIgnoreParentRotation;
}

void MeshComponent::IgnoreParentRotation(bool b) {
    m_bIgnoreParentRotation = b;
    UpdateWorldMatrix();
}

bool MeshComponent::IsIgnoreParentPosition() {
    return m_bIgnoreParentPosition;
}

void MeshComponent::IgnoreParentPosition(bool b) {
    m_bIgnoreParentPosition = b;
    UpdateWorldMatrix();
}

bool MeshComponent::IsIgnoreParentScaling() {
    return m_bIgnoreParentScaling;
}

void MeshComponent::IgnoreParentScaling(bool b) {
    m_bIgnoreParentScaling = b;
    UpdateWorldMatrix();
}

void MeshComponent::SetCreationData(MeshComponentCreationData data) {
    m_xData = data;
    UpdateWorldMatrix();
}

MeshComponentCreationData MeshComponent::GetCreationData() {
    return m_xData;
}

void MeshComponent::UpdateWorldMatrix() {
    dx::XMMATRIX scaleMat = dx::XMMatrixScaling(m_xScaling.x, m_xScaling.y, m_xScaling.z);
    dx::XMMATRIX rotXMat = dx::XMMatrixRotationX(m_xRotation.x);
    dx::XMMATRIX rotYMat = dx::XMMatrixRotationY(m_xRotation.y);
    dx::XMMATRIX rotZMat = dx::XMMatrixRotationZ(m_xRotation.z);
    dx::XMMATRIX transMat = dx::XMMatrixTranslation(m_xPosition.x, m_xPosition.y, m_xPosition.z);

    auto localMatrix = scaleMat * rotXMat * rotYMat * rotZMat * transMat;
    
    dx::XMMATRIX parentScaleMat = dx::XMMatrixIdentity();
    dx::XMMATRIX parentRotMat = dx::XMMatrixIdentity();
    dx::XMMATRIX parentTransMat = dx::XMMatrixIdentity();

    if (!m_bIgnoreParentScaling) {
        parentScaleMat = dx::XMMatrixScaling(m_xParentScaling.x, m_xParentScaling.y, m_xParentScaling.z);
    }

    if (!m_bIgnoreParentRotation) {
        dx::XMMATRIX pRotX = dx::XMMatrixRotationX(m_xParentRotation.x);
        dx::XMMATRIX pRotY = dx::XMMatrixRotationY(m_xParentRotation.y);
        dx::XMMATRIX pRotZ = dx::XMMatrixRotationZ(m_xParentRotation.z);
        parentRotMat = pRotX * pRotY * pRotZ;
    }

    if (!m_bIgnoreParentPosition) {
        parentTransMat = dx::XMMatrixTranslation(m_xParentPosition.x, m_xParentPosition.y, m_xParentPosition.z);
    }

    auto parentMatrix = parentScaleMat * parentRotMat * parentTransMat;

    m_xWorldMatrix = localMatrix * parentMatrix;

    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::UpdateTLAS);
}

void MeshComponent::SetMaterialStruct(MeshComponentMaterialData data) {
    m_xData.MaterialCBufferData = data;
}

MeshComponentMaterialData MeshComponent::GetMaterialStruct() {
    return m_xData.MaterialCBufferData;
}


void MeshComponent::SetPosition(const dx::XMFLOAT3& pos) {
    m_xPosition = pos;
    UpdateWorldMatrix();
}

void MeshComponent::SetRotation(const dx::XMFLOAT3& rot) {
    m_xRotation = rot;
    UpdateWorldMatrix();
}

void MeshComponent::SetScale(const dx::XMFLOAT3& scale) {
    m_xScaling = scale;
    UpdateWorldMatrix();
}

void MeshComponent::AddPosition(const dx::XMFLOAT3& delta) {
    m_xPosition.x += delta.x;
    m_xPosition.y += delta.y;
    m_xPosition.z += delta.z;
    UpdateWorldMatrix();
}

void MeshComponent::AddRotation(const dx::XMFLOAT3& delta) {
    m_xRotation.x += delta.x;
    m_xRotation.y += delta.y;
    m_xRotation.z += delta.z;
    UpdateWorldMatrix();
}

void MeshComponent::AddScale(const dx::XMFLOAT3& delta) {
    m_xScaling.x += delta.x;
    m_xScaling.y += delta.y;
    m_xScaling.z += delta.z;
    UpdateWorldMatrix();
}

dx::XMFLOAT3 MeshComponent::GetPosition() const { return m_xPosition; }
dx::XMFLOAT3 MeshComponent::GetRotation() const { return m_xRotation; }
dx::XMFLOAT3 MeshComponent::GetScale()    const { return m_xScaling; }

void MeshComponent::Init() {
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::MeshActivationDeactivation);
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::UpdateTLAS);
}

void MeshComponent::Destroy() {
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::MeshActivationDeactivation);
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::UpdateTLAS);
}

void MeshComponent::Activate(bool a) {
    RenderComponent::Activate(a);
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::MeshActivationDeactivation);
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::UpdateTLAS);
}

FD3DW::AccelerationStructureBuffers MeshComponent::GetBLASBuffer() {
    return m_xBLASBuffer;
}

void MeshComponent::OnStartRenderTick(const RenderComponentBeforeRenderInputData& data) {
    MeshComponentMatricesData cmb;
    cmb.Projection = dx::XMMatrixTranspose(data.Projection);
    cmb.View = dx::XMMatrixTranspose(data.View);
    cmb.World = dx::XMMatrixTranspose(m_xWorldMatrix);
    auto isBoneActive = m_xData.IsBoneActive.lock();
    cmb.IsActiveAnimation = isBoneActive ? *isBoneActive : false;
    cmb.CameraPosition = data.CameraPosition;

    m_pMatricesBuffer->CpyData(0, cmb);

    m_pMaterialBuffer->CpyData(0, m_xData.MaterialCBufferData);
}

void MeshComponent::OnRenderPreDepthPass(ID3D12GraphicsCommandList* list) {
    list->SetGraphicsRootShaderResourceView(PRE_DEPTH_ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG, GetEmptyStructuredBufferGPUVirtualAddress());

    list->IASetVertexBuffers(0, 1, m_xData.VertexBufferView);
    list->IASetIndexBuffer(m_xData.IndexBufferView);
    list->SetGraphicsRootConstantBufferView(PRE_DEPTH_CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG, m_pMatricesBuffer->GetGPULocation(0));

    list->DrawIndexedInstanced(m_xData.ObjectDescriptor.IndicesCount, 1, m_xData.ObjectDescriptor.IndicesOffset, m_xData.ObjectDescriptor.VerticesOffset, 0);
}

void MeshComponent::RenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
    m_pMatricesBuffer = FD3DW::UploadBuffer<MeshComponentMatricesData>::CreateConstantBuffer(device, 1);
    m_pMaterialBuffer = FD3DW::UploadBuffer<MeshComponentMaterialData>::CreateConstantBuffer(device, 1);
}

void MeshComponent::RenderInitDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) {
    auto geometry = GenerateGeometry();
    m_xBLASBuffer = FD3DW::CreateBottomLevelAS(device, list, { geometry }, BASE_RENDERABLE_OBJECTS_BLAS_HIT_GROUP_INDEX, false).front();
}

void MeshComponent::UpdateBLASDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) {
    auto geometry = GenerateGeometry();
    FD3DW::UpdateBottomLevelAS(device, list, m_xBLASBuffer, { geometry });
}

void MeshComponent::OnEndRenderTick(ID3D12GraphicsCommandList* list) {}

IndirectRenderDataPair MeshComponent::GetIndirectRenderDataPair() {
    IndirectMeshRenderData data;
    data.CBMaterials = m_pMaterialBuffer->GetGPULocation(0);
    data.CBMatrices = m_pMatricesBuffer->GetGPULocation(0);
    data.SRVBones = m_xData.BoneBuffer ? m_xData.BoneBuffer->GetResource()->GetGPUVirtualAddress() : GetEmptyStructuredBufferGPUVirtualAddress();
    data.VertexBufferView = *m_xData.VertexBufferView;
    data.IndexBufferView = *m_xData.IndexBufferView;
    data.DrawArguments.IndexCountPerInstance = m_xData.ObjectDescriptor.IndicesCount;
    data.DrawArguments.InstanceCount = 1u;
    data.DrawArguments.StartIndexLocation = m_xData.ObjectDescriptor.IndicesOffset;
    data.DrawArguments.BaseVertexLocation = (INT)m_xData.ObjectDescriptor.VerticesOffset;
    data.DrawArguments.StartInstanceLocation = 0u;

    MeshAABBInstanceData instanceData;

    auto [min, max] = GetBoundingBoxFromObjectDesc(m_xData.ObjectDescriptor, m_xWorldMatrix);
    instanceData.MinP = min;
    instanceData.MaxP = max;

    return { data,instanceData };
}


std::pair<dx::XMFLOAT3, dx::XMFLOAT3> MeshComponent::GetBoundingBoxFromObjectDesc(FD3DW::ObjectDesc desc, dx::XMMATRIX world) {
    std::pair<dx::XMFLOAT3, dx::XMFLOAT3> ret;

    dx::XMFLOAT3 localMin = desc.ObjectMin;
    dx::XMFLOAT3 localMax = desc.ObjectMax;

    dx::XMFLOAT3 corners[8] = {
        { localMin.x, localMin.y, localMin.z },
        { localMax.x, localMin.y, localMin.z },
        { localMin.x, localMax.y, localMin.z },
        { localMax.x, localMax.y, localMin.z },
        { localMin.x, localMin.y, localMax.z },
        { localMax.x, localMin.y, localMax.z },
        { localMin.x, localMax.y, localMax.z },
        { localMax.x, localMax.y, localMax.z }
    };

    dx::XMFLOAT3 transformed[8];
    for (int i = 0; i < 8; i++) {
        dx::XMVECTOR v = dx::XMLoadFloat3(&corners[i]);
        v = dx::XMVector3TransformCoord(v, world);
        dx::XMStoreFloat3(&transformed[i], v);
    }

    dx::XMFLOAT3 worldMin = transformed[0];
    dx::XMFLOAT3 worldMax = transformed[0];

    for (int i = 1; i < 8; i++) {
        worldMin.x = std::min(worldMin.x, transformed[i].x);
        worldMin.y = std::min(worldMin.y, transformed[i].y);
        worldMin.z = std::min(worldMin.z, transformed[i].z);

        worldMax.x = std::max(worldMax.x, transformed[i].x);
        worldMax.y = std::max(worldMax.y, transformed[i].y);
        worldMax.z = std::max(worldMax.z, transformed[i].z);
    }

    ret.first = worldMin;
    ret.second = worldMax;
    return ret;
}

FD3DW::AccelerationStructureInput MeshComponent::GenerateGeometry() {
    FD3DW::AccelerationStructureInput geometry;
    geometry.indexBuffer = m_xData.IndexBuffer;
    geometry.vertexBuffer = m_xData.VertexBuffer;
    geometry.indexCount = m_xData.ObjectDescriptor.IndicesCount;
    geometry.indexFormat = DEFAULT_INDEX_BUFFER_FORMAT;
    geometry.indexOffset = m_xData.ObjectDescriptor.IndicesOffset;
    geometry.indexStride = FD3DW::GetFormatSizeInBytes(DEFAULT_INDEX_BUFFER_FORMAT);
    geometry.vertexFormat = DEFAULT_RT_VERTEX_BUFFER_FORMAT;
    geometry.vertexCount = m_xData.ObjectDescriptor.VerticesCount;
    geometry.vertexOffset = m_xData.ObjectDescriptor.VerticesOffset;
    geometry.vertexStride = (UINT)m_xData.VertexStructSize;
    return geometry;
}
