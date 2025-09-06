#pragma once

#include <pch.h>
#include <Entity/RenderObject/TRender.h>
#include <Component/RenderObject/MeshComponent.h>

class TMesh : public TRender {
public:
	TMesh() = default;
	virtual ~TMesh() = default;

public:
	BEGIN_FIELD_REGISTRATION(TMesh, TRender)
		REGISTER_FIELD(m_bIsCreated);
		REGISTER_FIELD(m_bIsCreatedDXR);
	END_FIELD_REGISTRATION();


public:
	virtual void DoRenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;

	virtual void BeforeRenderInitAfterCreation(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;
	virtual void BeforeRenderInitAfterLoad(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;

protected:
	bool m_bIsCreated = false;
	bool m_bIsCreatedDXR = false;



};