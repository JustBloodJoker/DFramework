#pragma once

#include <pch.h>
#include <Entity/Core/ComponentHolder.h>
#include <MainRenderer/GlobalRenderThreadManager.h>

class TRender : public ComponentHolder {
public:
	TRender() = default;
	virtual ~TRender() = default;


public:
    REFLECT_BODY(TRender)
    BEGIN_REFLECT(TRender, ComponentHolder)
    END_REFLECT(TRender)

public:
	std::shared_ptr<FD3DW::ExecutionHandle> RenderInit(ID3D12Device* device, std::shared_ptr<FD3DW::ExecutionHandle> sync);
	std::shared_ptr<FD3DW::ExecutionHandle> RenderInitDXR(ID3D12Device5* device, std::shared_ptr<FD3DW::ExecutionHandle> sync);

public:
	virtual void DoRenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list);
	virtual void DoRenderInitDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list);

};