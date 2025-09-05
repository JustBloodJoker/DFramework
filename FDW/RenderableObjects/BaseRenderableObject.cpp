#include <RenderableObjects/BaseRenderableObject.h>
#include "RenderableMesh.h"



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
dx::XMMATRIX BaseRenderableObject::GetWorldMatrix() const { return m_xWorldMatrix; }

const std::string& BaseRenderableObject::GetName() const { return m_sName; }

bool BaseRenderableObject::IsCanRenderInPass(RenderPass pass) {
    return (GetRenderPass() & pass) == pass;
}

bool BaseRenderableObject::IsNeedUpdateTLAS() {
    return m_bIsNeedUpdateTLAS;
}

void BaseRenderableObject::AfterTLASUpdate() {
    m_bIsNeedUpdateTLAS = false;
}

std::vector<std::pair<FD3DW::AccelerationStructureBuffers, dx::XMMATRIX>> BaseRenderableObject::GetBLASInstances()
{
    std::vector<std::pair<FD3DW::AccelerationStructureBuffers, dx::XMMATRIX>> ret;

    for (const auto& buffer : m_vBLASBuffers) {
        ret.push_back({ buffer , m_xWorldMatrix });
    }

    return ret;
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

std::pair<dx::XMFLOAT3, float> BaseRenderableObject::GetBoundingSphereFromObjectDesc(FD3DW::ObjectDesc desc, dx::XMMATRIX world) {
    std::pair<dx::XMFLOAT3, float> ret;

    dx::XMFLOAT3 center;
    center.x = (desc.ObjectMin.x + desc.ObjectMax.x) * 0.5f;
    center.y = (desc.ObjectMin.y + desc.ObjectMax.y) * 0.5f;
    center.z = (desc.ObjectMin.z + desc.ObjectMax.z) * 0.5f;

    dx::XMFLOAT3 extents;
    extents.x = (desc.ObjectMax.x - desc.ObjectMin.x) * 0.5f;
    extents.y = (desc.ObjectMax.y - desc.ObjectMin.y) * 0.5f;
    extents.z = (desc.ObjectMax.z - desc.ObjectMin.z) * 0.5f;

    dx::XMVECTOR vecCenter = dx::XMLoadFloat3(&center);
    vecCenter = dx::XMVector3Transform(vecCenter, world);
    dx::XMStoreFloat3(&ret.first, vecCenter);

    dx::XMVECTOR scaleVec;
    scaleVec = dx::XMVectorSet(extents.x, extents.y, extents.z, 0.0f);
    scaleVec = dx::XMVector3Transform(scaleVec, world);

    dx::XMFLOAT3 scaledExtents;
    dx::XMStoreFloat3(&scaledExtents, scaleVec);
    ret.second = sqrtf(scaledExtents.x * scaledExtents.x + scaledExtents.y * scaledExtents.y + scaledExtents.z * scaledExtents.z);

    return ret;
}

std::pair<dx::XMFLOAT3, dx::XMFLOAT3> BaseRenderableObject::GetBoundingBoxFromObjectDesc(FD3DW::ObjectDesc desc, dx::XMMATRIX world) {
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

void BaseRenderableObject::UpdateWorldMatrix() {
    
    dx::XMMATRIX scaleMat = dx::XMMatrixScaling(m_xScaling.x, m_xScaling.y, m_xScaling.z);
    dx::XMMATRIX rotXMat = dx::XMMatrixRotationX(m_xRotation.x);
    dx::XMMATRIX rotYMat = dx::XMMatrixRotationY(m_xRotation.y);
    dx::XMMATRIX rotZMat = dx::XMMatrixRotationZ(m_xRotation.z);
    dx::XMMATRIX transMat = dx::XMMatrixTranslation(m_xPosition.x, m_xPosition.y, m_xPosition.z);

    m_xWorldMatrix = scaleMat * rotXMat * rotYMat * rotZMat * transMat;
    m_bIsNeedUpdateTLAS = true;
}