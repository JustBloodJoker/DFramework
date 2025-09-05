#pragma once 

#include <pch.h>
#include <Component/Core/IComponent.h>
#include <MainRenderer/GlobalRenderThreadManager.h>


struct RenderComponentBeforeRenderInputData {
    float Time;
    float DT;
    dx::XMMATRIX Projection;
    dx::XMMATRIX View;
    dx::XMFLOAT3 CameraPosition;
    ID3D12Device* Device;
    ID3D12GraphicsCommandList* CommandList;
};

class RenderComponent : public IComponent {
public:
	RenderComponent() = default;
	virtual ~RenderComponent() = default;

public:
	BEGIN_FIELD_REGISTRATION(RenderComponent, IComponent)
        REGISTER_FIELD(m_xWorldMatrix)
	END_FIELD_REGISTRATION();

public:
    dx::XMMATRIX GetWorldMatrix() const;

	virtual void OnStartRenderTick(const RenderComponentBeforeRenderInputData& data) = 0;
    virtual void RenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;
    virtual void RenderInitDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list);
    virtual void OnEndRenderTick(ID3D12GraphicsCommandList* list) = 0;

protected:
    dx::XMMATRIX m_xWorldMatrix = dx::XMMatrixIdentity();
};