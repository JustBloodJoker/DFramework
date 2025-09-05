#pragma once

#include <pch.h>
#include <Entity/RenderObject/RenderComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/Objects/Object.h>
#include <D3DFramework/Objects/RTObjectHelper.h>
#include <Entity/RenderObject/MeshesIndirectRenderData.h>



class MeshComponent : public RenderComponent {
public:
    MeshComponent() = default;
    virtual ~MeshComponent() = default;


public:
    BEGIN_FIELD_REGISTRATION(MeshComponent, RenderComponent)
        REGISTER_FIELD(m_xPosition);
        REGISTER_FIELD(m_xRotation);
        REGISTER_FIELD(m_xScaling);
        REGISTER_FIELD(m_iIndexInObject);
    END_FIELD_REGISTRATION();

public:

    void SetBonesBuffer(FD3DW::FResource* cmp);

    virtual void UpdateWorldMatrix();

    void SetPosition(const dx::XMFLOAT3& pos);
    void SetRotation(const dx::XMFLOAT3& rot);
    void SetScale(const dx::XMFLOAT3& scale);

    void AddPosition(const dx::XMFLOAT3& delta);
    void AddRotation(const dx::XMFLOAT3& delta);
    void AddScale(const dx::XMFLOAT3& delta);

    dx::XMFLOAT3 GetPosition() const;
    dx::XMFLOAT3 GetRotation() const;
    dx::XMFLOAT3 GetScale() const;

public:
    FD3DW::AccelerationStructureBuffers GetBLASBuffer();


public:
    virtual void OnStartRenderTick(const RenderComponentBeforeRenderInputData& data) override;
    virtual void OnRenderPreDepthPass(ID3D12GraphicsCommandList* list);
    virtual std::shared_ptr<FD3DW::ExecutionHandle> RenderInit(ID3D12Device* device, std::shared_ptr<FD3DW::ExecutionHandle> sync) override;
    virtual void OnEndRenderTick(ID3D12GraphicsCommandList* list) override;
    virtual IndirectRenderDataPair GetIndirectRenderDataPair();

protected:
        
    dx::XMFLOAT3 m_xPosition = { 0, 0, 0 };
    dx::XMFLOAT3 m_xRotation = { 0, 0, 0 };
    dx::XMFLOAT3 m_xScaling = { 1, 1, 1 };

    FD3DW::FResource* m_pStructureBufferBones;
    FD3DW::Object* m_pObject = nullptr;
    int m_iIndexInObject = 0;

    FD3DW::AccelerationStructureBuffers m_xBLASBuffer;
};


