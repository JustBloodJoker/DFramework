#pragma once

#include <pch.h>
#include <Entity/RenderObject/TMesh.h>
#include <D3DFramework/Objects/SimpleObjects.h>

class TSimpleMesh : public TMesh {
public:
	TSimpleMesh() = default;
	virtual ~TSimpleMesh() = default;

public:
	BEGIN_FIELD_REGISTRATION(TSimpleMesh, TMesh)
		REGISTER_FIELD(m_mPathToTextures);
	END_FIELD_REGISTRATION();

public:

	virtual void BeforeRenderInitAfterCreation(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRenderInitAfterLoad(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;

	void SetupTexture(FD3DW::TextureType type, std::string pathTo, ID3D12Device* device, ID3D12GraphicsCommandList* list);
	void EraseTexture(FD3DW::TextureType type, ID3D12Device* device);
	ID3D12Resource* GetTexture(FD3DW::TextureType type);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSRV(FD3DW::TextureType type);

protected:
	void CallCreationObject(ID3D12Device* device, ID3D12GraphicsCommandList* list);
	virtual std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;

protected:
	std::map<FD3DW::TextureType, std::string> m_mPathToTextures;

	std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> m_pObject = nullptr;

	std::unique_ptr<FD3DW::Material> m_pMaterial = nullptr;
};
