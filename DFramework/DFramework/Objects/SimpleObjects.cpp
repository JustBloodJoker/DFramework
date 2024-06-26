#include "../pch.h"
#include "SimpleObjects.h"


namespace FDW
{

	Cube::Cube(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		std::vector<FDW::VertexFrameWork> vertices;
		std::vector<std::uint16_t> indices;

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(-1.0f, -1.0f, -1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(-1.0f, +1.0f, -1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(+1.0f, +1.0f, -1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(+1.0f, -1.0f, -1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(-1.0f, -1.0f, +1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(-1.0f, +1.0f, +1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(+1.0f, +1.0f, +1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(+1.0f, -1.0f, +1.0f);

		FDW::BufferMananger::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), vertices.size(), vertexBuffer, pVertexUploadBuffer);

		vertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		vertexBufferView->BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView->SizeInBytes = vertices.size() * sizeof(FDW::VertexFrameWork);
		vertexBufferView->StrideInBytes = sizeof(FDW::VertexFrameWork);

		indices =
		{
			0, 1, 2,
			0, 2, 3,

			4, 6, 5,
			4, 7, 6,

			4, 5, 1,
			4, 1, 0,

			3, 2, 6,
			3, 6, 7,

			1, 5, 6,
			1, 6, 2,

			4, 0, 3,
			4, 3, 7
		};

		FDW::BufferMananger::CreateDefaultBuffer<std::uint16_t>(pDevice, pCommandList, indices.data(), indices.size(), indexBuffer, pIndexUploadBuffer);

		indexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		indexBufferView->BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView->Format = DXGI_FORMAT_R16_UNORM;
		indexBufferView->SizeInBytes = indices.size() * sizeof(std::uint16_t);

		if (neverUpdate)
		{
			pIndexUploadBuffer.release();
			pVertexUploadBuffer.release();
		}

		objectParameters.emplace_back(ObjectDesc{ vertices.size(), 0, indices.size(), 0, 0 });
	}

	Rectangle::Rectangle(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		std::vector<FDW::VertexFrameWork> vertices;
		std::vector<std::uint16_t> indices;

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(-1.0f, -1.0f, 0.0f);
		(*vertices.rbegin()).texCoord = dx::XMFLOAT2(0.0f, 1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(1.0f, -1.0f, 0.0f);
		(*vertices.rbegin()).texCoord = dx::XMFLOAT2(1.0f, 1.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(-1.0f, +1.0f, 0.0f);
		(*vertices.rbegin()).texCoord = dx::XMFLOAT2(0.0f, 0.0f);

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(+1.0f, +1.0f, 0.0f);
		(*vertices.rbegin()).texCoord = dx::XMFLOAT2(1.0f, 0.0f);

		FDW::BufferMananger::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), vertices.size(), vertexBuffer, pVertexUploadBuffer);

		vertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		vertexBufferView->BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView->SizeInBytes = vertices.size() * sizeof(FDW::VertexFrameWork);
		vertexBufferView->StrideInBytes = sizeof(FDW::VertexFrameWork);

		indices =
		{
			0, 1, 2,
			1, 2, 3,
		};

		FDW::BufferMananger::CreateDefaultBuffer<std::uint16_t>(pDevice, pCommandList, indices.data(), indices.size(), indexBuffer, pIndexUploadBuffer);

		indexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		indexBufferView->BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView->Format = DXGI_FORMAT_R16_UNORM;
		indexBufferView->SizeInBytes = indices.size() * sizeof(std::uint16_t);

		if (neverUpdate)
		{
			pIndexUploadBuffer.release();
			pVertexUploadBuffer.release();
		}

		objectParameters.emplace_back(ObjectDesc{ vertices.size(), 0, indices.size(), 0, 0 });
	}

	Point::Point(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		std::vector<FDW::VertexFrameWork> vertices;
		std::vector<std::uint16_t> indices;

		vertices.emplace_back(FDW::VertexFrameWork());
		(*vertices.rbegin()).pos = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
		indices = { 0 };

		FDW::BufferMananger::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), vertices.size(), vertexBuffer, pVertexUploadBuffer);

		vertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		vertexBufferView->BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView->SizeInBytes = vertices.size() * sizeof(FDW::VertexFrameWork);
		vertexBufferView->StrideInBytes = sizeof(FDW::VertexFrameWork);

		FDW::BufferMananger::CreateDefaultBuffer<std::uint16_t>(pDevice, pCommandList, indices.data(), indices.size(), indexBuffer, pIndexUploadBuffer);

		indexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		indexBufferView->BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView->Format = DXGI_FORMAT_R16_UNORM;
		indexBufferView->SizeInBytes = indices.size() * sizeof(std::uint16_t);

		if (neverUpdate)
		{
			pIndexUploadBuffer.release();
			pVertexUploadBuffer.release();
		}

		objectParameters.emplace_back(ObjectDesc{ vertices.size(), 0, indices.size(), 0, 0 });
	}

}