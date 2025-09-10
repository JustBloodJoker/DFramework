#pragma once

#include <pch.h>
#include <Entity/RenderObject/TMesh.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>


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

	//////////////////////////////////////////////////
	//////   MATERIAL DATA & TEXTURE GET SET

    #define IMPL_XMFLOAT4_GET_SET_FUNCTION(name)                         \
        virtual dx::XMFLOAT4 Get##name() {                               \
            auto component = GetComponent<MeshComponent>();              \
            if (!component) return dx::XMFLOAT4(0, 0, 0, 0);             \
            auto matData = component->GetMaterialStruct();               \
            return matData.name;                                         \
        }                                                                \
        virtual void Set##name(dx::XMFLOAT4 data) {                      \
            auto component = GetComponent<MeshComponent>();              \
            if (!component) return;                                      \
            auto matData = component->GetMaterialStruct();               \
            matData.name = data;                                         \
            component->SetMaterialStruct(matData);                       \
        }

    #define IMPL_FLOAT_GET_SET_FUNCTION(name)                            \
        virtual float Get##name() {                                      \
            auto component = GetComponent<MeshComponent>();              \
            if (!component) return 0.0f;                                 \
            auto matData = component->GetMaterialStruct();               \
            return matData.name;                                         \
        }                                                                \
        virtual void Set##name(float data) {                             \
            auto component = GetComponent<MeshComponent>();              \
            if (!component) return;                                      \
            auto matData = component->GetMaterialStruct();               \
            matData.name = data;                                         \
            component->SetMaterialStruct(matData);                       \
        }

	IMPL_XMFLOAT4_GET_SET_FUNCTION(Diffuse);
	IMPL_XMFLOAT4_GET_SET_FUNCTION(Ambient);
	IMPL_XMFLOAT4_GET_SET_FUNCTION(Emissive);
	IMPL_XMFLOAT4_GET_SET_FUNCTION(Specular);

	IMPL_FLOAT_GET_SET_FUNCTION(Roughness);
	IMPL_FLOAT_GET_SET_FUNCTION(Metalness);
	IMPL_FLOAT_GET_SET_FUNCTION(SpecularPower);
	IMPL_FLOAT_GET_SET_FUNCTION(HeightScale);

    #undef IMPL_XMFLOAT4_GET_SET_FUNCTION
    #undef IMPL_FLOAT_GET_SET_FUNCTION

	//////////////////////////////////////////////////

	void SetupTexture(FD3DW::TextureType type, std::string pathTo, ID3D12Device* device, ID3D12GraphicsCommandList* list);
	void EraseTexture(FD3DW::TextureType type, ID3D12Device* device);
	ID3D12Resource* GetTexture(FD3DW::TextureType type);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSRV(FD3DW::TextureType type);

protected:
	void CallCreationObject(ID3D12Device* device, ID3D12GraphicsCommandList* list);
	virtual std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;

protected:
	std::map<FD3DW::TextureType, std::string> m_mPathToTextures;


    std::unique_ptr<FD3DW::ObjectVertexIndexDataCreator<FD3DW::SceneVertexFrameWork>> m_pObjectVBV_IBV;
	std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> m_pObject = nullptr;

	std::unique_ptr<FD3DW::Material> m_pMaterial = nullptr;
};
