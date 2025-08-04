#pragma once
#include "Object.h"

namespace FD3DW
{
	template<typename TVertex>
	class SimpleObject : public Object
	{
	public:
		using GeneratorFunc = std::function<void(std::vector<TVertex>&, std::vector<std::uint32_t>&)>;

		SimpleObject(ID3D12Device* pDevice,ID3D12GraphicsCommandList* pCommandList,GeneratorFunc generator)
		{
			std::vector<TVertex> vertices;
			std::vector<std::uint32_t> indices;
			generator(vertices, indices);

			BufferManager::CreateDefaultBuffer<TVertex>(pDevice, pCommandList, vertices.data(), static_cast<UINT>(vertices.size()),m_pVertexBuffer, m_pVertexUploadBuffer);

			m_pVertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
			m_pVertexBufferView->BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
			m_pVertexBufferView->SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(TVertex));
			m_pVertexBufferView->StrideInBytes = sizeof(TVertex);

			BufferManager::CreateDefaultBuffer<std::uint32_t>(pDevice, pCommandList, indices.data(),static_cast<UINT>(indices.size()),m_pIndexBuffer, m_pIndexUploadBuffer);

			m_pIndexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
			m_pIndexBufferView->BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
			m_pIndexBufferView->Format = DEFAULT_INDEX_BUFFER_FORMAT;
			m_pIndexBufferView->SizeInBytes = static_cast<UINT>(indices.size() * sizeof(std::uint32_t));
			
			m_vObjectParameters.emplace_back(ObjectDesc{vertices.size(), 0,indices.size(), 0, 0});
		}

		virtual size_t GetVertexStructSize() const override {
			return sizeof(TVertex);
		}

		virtual ~SimpleObject() = default;

	protected:
		std::unique_ptr<UploadBuffer<TVertex>> m_pVertexUploadBuffer;
		std::unique_ptr<UploadBuffer<std::uint32_t>> m_pIndexUploadBuffer;
	};

	void GenerateCube(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
	void GenerateRectangle(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
	void GeneratePoint(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);

	//////////////////////////////////////
	////			DEFAULT TYPES
	class Cube : public SimpleObject<VertexFrameWork>
	{
	public:
		Cube(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
			: SimpleObject(device, cmdList, GenerateCube)
		{
		}
	};

	class Rectangle : public SimpleObject<VertexFrameWork>
	{
	public:
		Rectangle(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
			: SimpleObject(device, cmdList, GenerateRectangle)
		{
		}
	};

	class Point : public SimpleObject<VertexFrameWork>
	{
	public:
		Point(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
			: SimpleObject(device, cmdList, GeneratePoint)
		{
		}
	};

}