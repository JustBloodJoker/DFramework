#include <RenderableObjects/BaseRenderableObject.h>

BaseRenderableObject::BaseRenderableObject(const std::string& name)
    : m_sName(name)
{
    UpdateWorldMatrix();
}

void BaseRenderableObject::SetPosition(const dx::XMFLOAT3& pos) {
    m_xPosition = pos;
    UpdateWorldMatrix();
}

void BaseRenderableObject::SetRotation(const dx::XMFLOAT3& rot) {
    m_xRotation = rot;
    UpdateWorldMatrix();
}

void BaseRenderableObject::SetScale(const dx::XMFLOAT3& scale) {
    m_xScaling = scale;
    UpdateWorldMatrix();
}

void BaseRenderableObject::AddPosition(const dx::XMFLOAT3& delta) {
    m_xPosition.x += delta.x;
    m_xPosition.y += delta.y;
    m_xPosition.z += delta.z;
    UpdateWorldMatrix();
}

void BaseRenderableObject::AddRotation(const dx::XMFLOAT3& delta) {
    m_xRotation.x += delta.x;
    m_xRotation.y += delta.y;
    m_xRotation.z += delta.z;
    UpdateWorldMatrix();
}

void BaseRenderableObject::AddScale(const dx::XMFLOAT3& delta) {
    m_xScaling.x += delta.x;
    m_xScaling.y += delta.y;
    m_xScaling.z += delta.z;
    UpdateWorldMatrix();
}

dx::XMFLOAT3 BaseRenderableObject::GetPosition() const { return m_xPosition; }
dx::XMFLOAT3 BaseRenderableObject::GetRotation() const { return m_xRotation; }
dx::XMFLOAT3 BaseRenderableObject::GetScale()    const { return m_xScaling; }

const std::string& BaseRenderableObject::GetName() const { return m_sName; }

bool BaseRenderableObject::IsCanRenderInPass(RenderPass pass) {
    return (GetRenderPass() & pass) == pass;
}

UINT BaseRenderableObject::GetIndexSize(FD3DW::Object* obj, const size_t index)
{
    return (UINT)obj->GetObjectParameters(index).IndicesCount;
}

UINT BaseRenderableObject::GetIndexStartPos(FD3DW::Object* obj, const size_t index)
{
    return (UINT)obj->GetObjectParameters(index).IndicesOffset;
}

UINT BaseRenderableObject::GetVertexStartPos(FD3DW::Object* obj, const size_t index)
{
    return (UINT)obj->GetObjectParameters(index).VerticesOffset;
}

UINT BaseRenderableObject::GetVertexSize(FD3DW::Object* obj, const size_t index)
{
    return (UINT)obj->GetObjectParameters(index).VerticesCount;
}

UINT BaseRenderableObject::GetMaterialIndex(FD3DW::Object* obj, const size_t index)
{
    return (UINT)obj->GetObjectParameters(index).MaterialIndex;
}

void BaseRenderableObject::UpdateWorldMatrix() {
    
    dx::XMMATRIX scaleMat = dx::XMMatrixScaling(m_xScaling.x, m_xScaling.y, m_xScaling.z);
    dx::XMMATRIX rotXMat = dx::XMMatrixRotationX(m_xRotation.x);
    dx::XMMATRIX rotYMat = dx::XMMatrixRotationY(m_xRotation.y);
    dx::XMMATRIX rotZMat = dx::XMMatrixRotationZ(m_xRotation.z);
    dx::XMMATRIX transMat = dx::XMMatrixTranslation(m_xPosition.x, m_xPosition.y, m_xPosition.z);

    m_xWorldMatrix = scaleMat * rotXMat * rotYMat * rotZMat * transMat;
}