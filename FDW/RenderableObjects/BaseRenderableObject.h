#pragma once

#include <pch.h>
#include <D3DFramework/Objects/Object.h>

enum RenderPass
{
    None = 0x0,
    Deferred = 0x1,
    Forward = 0x2,
    DeferredAndForward = 0x3,
};

struct BeforeRenderInputData {
    float Time;
    float DT;
    dx::XMMATRIX Projection;
    dx::XMMATRIX View;
    dx::XMMATRIX AdditionalWorld;
    dx::XMFLOAT3 CameraPosition;
    ID3D12Device* Device;
    ID3D12GraphicsCommandList* CommandList;
};

class BaseRenderableObject {

public:
    static void CreateEmptyStructuredBuffer(ID3D12Device* device);
    D3D12_GPU_VIRTUAL_ADDRESS GetEmptyStructuredBufferGPUVirtualAddress();

public:

    BaseRenderableObject(const std::string& name);
    virtual ~BaseRenderableObject() = default;

    virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;
    virtual void BeforeRender(const BeforeRenderInputData& data) = 0;
    virtual void DeferredRender(ID3D12GraphicsCommandList* list) = 0;
    virtual void ForwardRender(ID3D12GraphicsCommandList* list) = 0;
    virtual RenderPass GetRenderPass() const = 0;

    void SetPosition(const dx::XMFLOAT3& pos);
    void SetRotation(const dx::XMFLOAT3& rot);
    void SetScale(const dx::XMFLOAT3& scale);

    void AddPosition(const dx::XMFLOAT3& delta);
    void AddRotation(const dx::XMFLOAT3& delta);
    void AddScale(const dx::XMFLOAT3& delta);

    dx::XMFLOAT3 GetPosition() const;
    dx::XMFLOAT3 GetRotation() const;
    dx::XMFLOAT3 GetScale() const;

    const std::string& GetName() const;

    bool IsCanRenderInPass(RenderPass pass);

public:
    static UINT GetIndexSize(FD3DW::Object* obj, const size_t index);
    static UINT GetIndexStartPos(FD3DW::Object* obj, const size_t index);
    static UINT GetVertexStartPos(FD3DW::Object* obj, const size_t index);
    static UINT GetVertexSize(FD3DW::Object* obj, const size_t index);
    static UINT GetMaterialIndex(FD3DW::Object* obj, const size_t index);

protected:
    void UpdateWorldMatrix();

protected:
    dx::XMMATRIX m_xWorldMatrix = dx::XMMatrixIdentity();

    dx::XMFLOAT3 m_xPosition = { 0, 0, 0 };
    dx::XMFLOAT3 m_xRotation = { 0, 0, 0 };
    dx::XMFLOAT3 m_xScaling = { 1, 1, 1 };

    std::string m_sName;
};