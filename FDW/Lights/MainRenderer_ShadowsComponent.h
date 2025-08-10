#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <Lights/ShadowsStructures.h>
#include <D3DFramework/GraphicUtilites/FResource.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

class MainRenderer;

class MainRenderer_ShadowsComponent : public MainRendererComponent {

public:
	MainRenderer_ShadowsComponent() = default;
	virtual ~MainRenderer_ShadowsComponent() = default;

	virtual bool IsCanBeEnabled(MainRenderer* renderer) = 0;
	virtual void BeforeGBufferPass() = 0;
	virtual void AfterGBufferPass() = 0;
	virtual D3D12_SRV_DIMENSION GetSRVResultDimension() = 0;
	
	virtual void BindResultResource(ID3D12Device* device, FD3DW::SRV_UAVPacker* srv, size_t index);
	virtual FD3DW::FResource* GetResultResource()=0;

	virtual void BeforeRender(ID3D12GraphicsCommandList* list);

	virtual ShadowType Type();
public:
	BEGIN_FIELD_REGISTRATION(MainRenderer_ShadowsComponent, MainRendererComponent)
	END_FIELD_REGISTRATION();

protected:
	

};