#pragma once

#include "../pch.h"
#include "../GraphicUtilites/BufferManager.h"

namespace FD3DW {

class IObjectVertexIndexDataCreator {
public:
	IObjectVertexIndexDataCreator() = default;
	virtual ~IObjectVertexIndexDataCreator() = default;

	virtual UINT GetVertexStructSize()=0;
	virtual ID3D12Resource* GetVertexBufferResource()=0;
	virtual ID3D12Resource* GetIndexBufferResource()=0;
	virtual D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView()=0;
	virtual D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView()=0;
};

template<typename TVertex, typename TIndex = std::uint32_t>
class ObjectVertexIndexDataCreator : public IObjectVertexIndexDataCreator {
public:
    ObjectVertexIndexDataCreator() = default;
    virtual ~ObjectVertexIndexDataCreator() = default;

    void Create(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::vector<TVertex>& vertices, const std::vector<TIndex>& indices,bool neverUpdate = false)
    {
        BufferManager::CreateDefaultBuffer(pDevice,pCommandList,vertices.data(),(UINT)vertices.size(),VertexBufferResource, VertexUploadBuffer );

        VertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
        VertexBufferView->BufferLocation = VertexBufferResource->GetGPUVirtualAddress();
        VertexBufferView->SizeInBytes = (UINT)(vertices.size() * sizeof(TVertex));
        VertexBufferView->StrideInBytes = sizeof(TVertex);

        BufferManager::CreateDefaultBuffer(pDevice,pCommandList,indices.data(),(UINT)indices.size() ,IndexBufferResource,IndexUploadBuffer);

        IndexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
        IndexBufferView->BufferLocation = IndexBufferResource->GetGPUVirtualAddress();
        IndexBufferView->Format = sizeof(TIndex) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        IndexBufferView->SizeInBytes = (UINT)(indices.size() * sizeof(TIndex));

        if (neverUpdate) {
            IndexUploadBuffer.reset();
            VertexUploadBuffer.reset();
        }
    }

    template<typename TInVertex, typename Converter>
    void CreateWithConverter(ID3D12Device* pDevice,ID3D12GraphicsCommandList* pCommandList,const std::vector<TInVertex>& inVertices,const std::vector<TIndex>& indices,Converter&& converter, bool neverUpdate = false)
    {
        std::vector<TVertex> converted;
        converted.reserve(inVertices.size());
        for (auto& v : inVertices) {
            converted.push_back(converter(v));
        }

        Create(pDevice, pCommandList, converted, indices, neverUpdate);
    }

    UINT GetVertexStructSize() override { return sizeof(TVertex); }
    ID3D12Resource* GetVertexBufferResource() override { return VertexBufferResource.Get(); }
    ID3D12Resource* GetIndexBufferResource() override { return IndexBufferResource.Get(); }
    D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() override { return VertexBufferView.get(); }
    D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() override { return IndexBufferView.get(); }

protected:
    wrl::ComPtr<ID3D12Resource> VertexBufferResource;
    wrl::ComPtr<ID3D12Resource> IndexBufferResource;

    std::unique_ptr<FD3DW::UploadBuffer<TVertex>> VertexUploadBuffer;
    std::unique_ptr<FD3DW::UploadBuffer<TIndex>> IndexUploadBuffer;

    std::unique_ptr<D3D12_VERTEX_BUFFER_VIEW> VertexBufferView;
    std::unique_ptr<D3D12_INDEX_BUFFER_VIEW> IndexBufferView;
};



}