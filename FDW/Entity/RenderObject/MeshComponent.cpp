#include <Entity/RenderObject/MeshComponent.h>
#include <World/World.h>

void MeshComponent::SetBonesBuffer(FD3DW::FResource* cmp) {
    m_pStructureBufferBones = cmp;
}

void MeshComponent::UpdateWorldMatrix() {

    dx::XMMATRIX scaleMat = dx::XMMatrixScaling(m_xScaling.x, m_xScaling.y, m_xScaling.z);
    dx::XMMATRIX rotXMat = dx::XMMatrixRotationX(m_xRotation.x);
    dx::XMMATRIX rotYMat = dx::XMMatrixRotationY(m_xRotation.y);
    dx::XMMATRIX rotZMat = dx::XMMatrixRotationZ(m_xRotation.z);
    dx::XMMATRIX transMat = dx::XMMatrixTranslation(m_xPosition.x, m_xPosition.y, m_xPosition.z);

    m_xWorldMatrix = scaleMat * rotXMat * rotYMat * rotZMat * transMat;
    
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::UpdateTLAS);
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

FD3DW::AccelerationStructureBuffers MeshComponent::GetBLASBuffer() {
    return m_xBLASBuffer;
}

void MeshComponent::OnStartRenderTick(const RenderComponentBeforeRenderInputData& data) {
    

}

void MeshComponent::OnRenderPreDepthPass(ID3D12GraphicsCommandList* list) {

}

std::shared_ptr<FD3DW::ExecutionHandle> MeshComponent::RenderInit(ID3D12Device* device, std::shared_ptr<FD3DW::ExecutionHandle> sync) {
    return std::shared_ptr<FD3DW::ExecutionHandle>();
}

void MeshComponent::OnEndRenderTick(ID3D12GraphicsCommandList* list) {}

IndirectRenderDataPair MeshComponent::GetIndirectRenderDataPair() {
    IndirectRenderDataPair ret;
    
    return ret;
}

