#pragma once

#include <pch.h>
#include <Entity/RenderObject/TRender.h>
#include <Component/RenderObject/MeshComponent.h>

class TMesh : public TRender {
public:
	TMesh() = default;
	virtual ~TMesh() = default;

public:
    REFLECT_BODY(TMesh)
    BEGIN_REFLECT(TMesh, TRender)
        REFLECT_PROPERTY(m_bIsCreated)
        REFLECT_PROPERTY(m_xPosition)
        REFLECT_PROPERTY(m_xRotation)
        REFLECT_PROPERTY(m_xScaling)
    END_REFLECT(TMesh)


public:
	void SetPosition(const dx::XMFLOAT3& pos);
	void SetRotation(const dx::XMFLOAT3& rot);
	void SetScale(const dx::XMFLOAT3& scale);

	dx::XMFLOAT3 GetPosition() const;
	dx::XMFLOAT3 GetRotation() const;
	dx::XMFLOAT3 GetScale() const;

public:
	virtual void DoRenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;

	virtual void BeforeRenderInitAfterCreation(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;
	virtual void BeforeRenderInitAfterLoad(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;

protected:
	dx::XMFLOAT3 m_xPosition = { 0,0,0 };
	dx::XMFLOAT3 m_xRotation = { 0,0,0 };
	dx::XMFLOAT3 m_xScaling = {1,1,1};

	bool m_bIsCreated = false;
};