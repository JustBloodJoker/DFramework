#pragma once

#include <pch.h>
#include <Component/RenderObject/RenderComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/Objects/Object.h>
#include <D3DFramework/Objects/RTObjectHelper.h>
#include <Component/RenderObject/MeshesIndirectRenderData.h>
#include <Component/RenderObject/MeshComponentMaterialData.h>
#include <Component/RenderObject/MeshComponentMatricesData.h>

struct MeshComponentCreationData {
    MeshComponentMaterialData MaterialCBufferData;
    size_t ID;

    FD3DW::ObjectDesc ObjectDescriptor;
    FD3DW::FResource* BoneBuffer;
    std::weak_ptr<bool> IsBoneActive;
    ID3D12Resource* IndexBuffer;
    ID3D12Resource* VertexBuffer;
    D3D12_INDEX_BUFFER_VIEW* IndexBufferView;
    D3D12_VERTEX_BUFFER_VIEW* VertexBufferView;
    UINT VertexStructSize;

    REFLECT_STRUCT(MeshComponentCreationData)
    BEGIN_REFLECT(MeshComponentCreationData)
        REFLECT_PROPERTY(MaterialCBufferData);
        REFLECT_PROPERTY(ID);
    END_REFLECT(MeshComponentCreationData)
};


class MeshComponent : public RenderComponent {
public:
    MeshComponent();
    virtual ~MeshComponent() = default;


public:
    REFLECT_BODY(MeshComponent)
    BEGIN_REFLECT(MeshComponent, RenderComponent)
        REFLECT_PROPERTY(m_xPosition);
        REFLECT_PROPERTY(m_xRotation);
        REFLECT_PROPERTY(m_xScaling);
        REFLECT_PROPERTY(m_xData);
        REFLECT_PROPERTY(m_xParentPosition);
        REFLECT_PROPERTY(m_xParentRotation);
        REFLECT_PROPERTY(m_xParentScaling);
        REFLECT_PROPERTY(m_bIgnoreParentScaling);
        REFLECT_PROPERTY(m_bIgnoreParentRotation);
        REFLECT_PROPERTY(m_bIgnoreParentPosition);
    END_REFLECT(MeshComponent)

public:

    void SetParentPosition(dx::XMFLOAT3 parentPosition);
    void SetParentRotation(dx::XMFLOAT3 parentRotation);
    void SetParentScale(dx::XMFLOAT3 parentScale);

    bool IsIgnoreParentRotation();
    void IgnoreParentRotation(bool b);
    bool IsIgnoreParentPosition();
    void IgnoreParentPosition(bool b);
    bool IsIgnoreParentScaling();
    void IgnoreParentScaling(bool b);

    void SetCreationData(MeshComponentCreationData data);
    MeshComponentCreationData GetCreationData();

    virtual void UpdateWorldMatrix();

    void SetMaterialStruct(MeshComponentMaterialData data);
    MeshComponentMaterialData GetMaterialStruct();

    void SetPosition(const dx::XMFLOAT3& pos);
    void SetRotation(const dx::XMFLOAT3& rot);
    void SetScale(const dx::XMFLOAT3& scale);

    void AddPosition(const dx::XMFLOAT3& delta);
    void AddRotation(const dx::XMFLOAT3& delta);
    void AddScale(const dx::XMFLOAT3& delta);

    dx::XMFLOAT3 GetPosition() const;
    dx::XMFLOAT3 GetRotation() const;
    dx::XMFLOAT3 GetScale() const;


    virtual void Init() override;
    virtual void Destroy() override;
    virtual void Activate(bool a) override;

public:
    FD3DW::AccelerationStructureBuffers GetBLASBuffer();


public:
    virtual void OnStartRenderTick(const RenderComponentBeforeRenderInputData& data) override;
    virtual void OnRenderPreDepthPass(ID3D12GraphicsCommandList* list);
    virtual void RenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
    virtual void RenderInitDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) override;
    virtual void UpdateBLASDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list);
    virtual void OnEndRenderTick(ID3D12GraphicsCommandList* list) override;
    virtual IndirectRenderDataPair GetIndirectRenderDataPair();


    std::pair<dx::XMFLOAT3, dx::XMFLOAT3> GetBoundingBoxFromObjectDesc(FD3DW::ObjectDesc desc, dx::XMMATRIX world);

    FD3DW::AccelerationStructureInput GenerateGeometry();
    
protected:
        
    dx::XMFLOAT3 m_xPosition = { 0, 0, 0 };
    dx::XMFLOAT3 m_xRotation = { 0, 0, 0 };
    dx::XMFLOAT3 m_xScaling = { 1, 1, 1 };

    bool m_bIgnoreParentScaling = false;
    bool m_bIgnoreParentRotation = false;
    bool m_bIgnoreParentPosition = false;

    dx::XMFLOAT3 m_xParentPosition = { 0, 0, 0 };
    dx::XMFLOAT3 m_xParentRotation = { 0, 0, 0 };
    dx::XMFLOAT3 m_xParentScaling = { 1, 1, 1 };

    MeshComponentCreationData m_xData;

    FD3DW::AccelerationStructureBuffers m_xBLASBuffer;
    
    std::unique_ptr<FD3DW::UploadBuffer<MeshComponentMatricesData>> m_pMatricesBuffer;
    std::unique_ptr<FD3DW::UploadBuffer<MeshComponentMaterialData>> m_pMaterialBuffer;
};


