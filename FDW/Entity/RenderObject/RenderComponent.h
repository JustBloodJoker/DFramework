#pragma once 

#include <pch.h>
#include <Entity/Core/IComponent.h>
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
    virtual std::shared_ptr<FD3DW::ExecutionHandle> RenderInit(ID3D12Device* device, std::shared_ptr<FD3DW::ExecutionHandle> sync) = 0;
    virtual void OnEndRenderTick(ID3D12GraphicsCommandList* list) = 0;

protected:
    dx::XMMATRIX m_xWorldMatrix = dx::XMMatrixIdentity();
};