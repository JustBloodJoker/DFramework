#include "pch.h"

#include <Component/RenderObject/MeshComponent.h>
#include <Config/LightingConfig.h>
#include <Config/PipelineRegistry.h>
#include <Config/RendererLimits.h>
#include <Config/RenderFormats.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>
#include <Config/RendererResources.h>
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

void MeshComponent::SetCreationData(MeshComponentCreationData data) {
    m_xData = data;
    m_xData.MaterialCBufferData.LoadedTexture[MATERIAL_ALPHA_MODE_FLAG_POS] = static_cast<int>(m_xData.ObjectDescriptor.AlphaMode);
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

void MeshComponent::Init() {
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

bool MeshComponent::HasSupportedAlphaMask() const {
    return GetRTGeometrySupportClass() == NSRTGeometrySupportClass::AlphaTestedTriangles;
}

bool MeshComponent::IsDoubleSided() const {
    return m_xData.ObjectDescriptor.DoubleSided != 0u;
}

NSRTGeometryMaterialFacts MeshComponent::GetRTGeometryMaterialFacts(bool sharedBuffersCompatible) const {
    const auto& material = m_xData.MaterialCBufferData;
    NSRTGeometryMaterialFacts facts{};
    
    switch (m_xData.ObjectDescriptor.AlphaMode) {
    case FD3DW::MaterialAlphaMode::Opaque: 
        facts.AlphaMode = NSRTGeometryAlphaMode::Opaque; 
        break;
    case FD3DW::MaterialAlphaMode::Mask: 
        facts.AlphaMode = NSRTGeometryAlphaMode::Mask; 
        break;
    case FD3DW::MaterialAlphaMode::Blend: 
        facts.AlphaMode = NSRTGeometryAlphaMode::Blend; 
        break;
    default: facts.AlphaMode = 
        NSRTGeometryAlphaMode::Unknown; 
        break;
    }
    
    facts.HasBaseColorTexture = material.LoadedTexture[FD3DW::TEXTURETYPE::BASE] >= 0 ? 1u : 0u;
    facts.HasOpacityTexture = material.LoadedTexture[FD3DW::TEXTURETYPE::OPACITY] >= 0 ? 1u : 0u;
    facts.HasTextureCoordinates = m_xData.ObjectDescriptor.HasTextureCoordinates;
    facts.HasCompatibleSharedBuffers = sharedBuffersCompatible ? 1u : 0u;
    facts.AlphaCutoff = m_xData.ObjectDescriptor.AlphaCutoff;
    facts.OpacityFactor = material.Diffuse.w;
    return facts;
}

NSRTGeometrySupportClass MeshComponent::GetRTGeometrySupportClass(bool sharedBuffersCompatible) const {
    return ClassifyGeometryMaterial(GetRTGeometryMaterialFacts(sharedBuffersCompatible));
}

bool MeshComponent::HasCompleteBLAS() const {
    return m_xBLASBuffer.pResult != nullptr;
}

void MeshComponent::OnStartRenderTick(const RenderComponentBeforeRenderInputData& data) {
    MeshComponentMatricesData cmb;
    cmb.Projection = dx::XMMatrixTranspose(data.Projection);
    cmb.View = dx::XMMatrixTranspose(data.View);
    cmb.World = dx::XMMatrixTranspose(m_xWorldMatrix);
    cmb.CameraPosition = data.CameraPosition;
    m_pMatricesBuffer->CpyData(0, cmb);

    m_pMaterialBuffer->CpyData(0, m_xData.MaterialCBufferData);
}

void MeshComponent::RenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
    m_pMatricesBuffer = FD3DW::UploadBuffer<MeshComponentMatricesData>::CreateConstantBuffer(device, 1);
    m_pMaterialBuffer = FD3DW::UploadBuffer<MeshComponentMaterialData>::CreateConstantBuffer(device, 1);
}

void MeshComponent::RenderInitDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) {
    auto geometry = GenerateGeometry();
    m_xBLASBuffer = FD3DW::CreateBottomLevelAS(device, list, { geometry }, NSOfflineConfig::RendererLimits::BaseRenderableBlasHitGroupIndex, false).front();
}

IndirectMeshRenderData MeshComponent::GetIndirectRenderData() {
    IndirectMeshRenderData data;
    data.CBMaterials = m_pMaterialBuffer->GetGPULocation(0);
    data.CBMatrices = m_pMatricesBuffer->GetGPULocation(0);
    data.VertexBufferView = *m_xData.VertexBufferView;
    data.IndexBufferView = *m_xData.IndexBufferView;
    data.DrawArguments.IndexCountPerInstance = m_xData.ObjectDescriptor.IndicesCount;
    data.DrawArguments.InstanceCount = 1u;
    data.DrawArguments.StartIndexLocation = m_xData.ObjectDescriptor.IndicesOffset;
    data.DrawArguments.BaseVertexLocation = (INT)m_xData.ObjectDescriptor.VerticesOffset;
    data.DrawArguments.StartInstanceLocation = 0u;

	return data;
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
    geometry.flags = HasSupportedAlphaMask() ? D3D12_RAYTRACING_GEOMETRY_FLAG_NONE : D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    
    return geometry;
}
