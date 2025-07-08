#include "../pch.h"
#include "SimpleObjects.h"


namespace FD3DW
{

	Cube::Cube(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		std::vector<FD3DW::VertexFrameWork> vertices;
		std::vector<std::uint32_t> indices;

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(-1.0f, -1.0f, -1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(-1.0f, +1.0f, -1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(+1.0f, +1.0f, -1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(+1.0f, -1.0f, -1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(-1.0f, -1.0f, +1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(-1.0f, +1.0f, +1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(+1.0f, +1.0f, +1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(+1.0f, -1.0f, +1.0f);

		FD3DW::BufferManager::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), (UINT)vertices.size(), m_pVertexBuffer, m_pVertexUploadBuffer);

		m_pVertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		m_pVertexBufferView->BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
		m_pVertexBufferView->SizeInBytes = (UINT)vertices.size() * sizeof(FD3DW::VertexFrameWork);
		m_pVertexBufferView->StrideInBytes = sizeof(FD3DW::VertexFrameWork);

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

		FD3DW::BufferManager::CreateDefaultBuffer<std::uint32_t>(pDevice, pCommandList, indices.data(), (UINT)indices.size(), m_pIndexBuffer, m_pIndexUploadBuffer);

		m_pIndexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		m_pIndexBufferView->BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
		m_pIndexBufferView->Format =  DXGI_FORMAT_R32_UINT;
		m_pIndexBufferView->SizeInBytes = (UINT)indices.size() * sizeof(std::uint32_t);

		if (neverUpdate)
		{
			m_pIndexUploadBuffer.release();
			m_pVertexUploadBuffer.release();
		}

		m_vObjectParameters.emplace_back(ObjectDesc{ vertices.size(), 0, indices.size(), 0, 0 });
	}

	Rectangle::Rectangle(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		std::vector<FD3DW::VertexFrameWork> vertices;
		std::vector<std::uint32_t> indices;

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(-1.0f, -1.0f, 0.0f);
		(*vertices.rbegin()).TexCoord = dx::XMFLOAT2(0.0f, 1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(1.0f, -1.0f, 0.0f);
		(*vertices.rbegin()).TexCoord = dx::XMFLOAT2(1.0f, 1.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(-1.0f, +1.0f, 0.0f);
		(*vertices.rbegin()).TexCoord = dx::XMFLOAT2(0.0f, 0.0f);

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(+1.0f, +1.0f, 0.0f);
		(*vertices.rbegin()).TexCoord = dx::XMFLOAT2(1.0f, 0.0f);

		FD3DW::BufferManager::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), (UINT)vertices.size(), m_pVertexBuffer, m_pVertexUploadBuffer);

		m_pVertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		m_pVertexBufferView->BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
		m_pVertexBufferView->SizeInBytes = (UINT)vertices.size() * sizeof(FD3DW::VertexFrameWork);
		m_pVertexBufferView->StrideInBytes = sizeof(FD3DW::VertexFrameWork);

		indices =
		{
			0, 1, 2,
			1, 2, 3,
		};

		FD3DW::BufferManager::CreateDefaultBuffer<std::uint32_t>(pDevice, pCommandList, indices.data(), (UINT)indices.size(), m_pIndexBuffer, m_pIndexUploadBuffer);

		m_pIndexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		m_pIndexBufferView->BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
		m_pIndexBufferView->Format =  DXGI_FORMAT_R32_UINT;
		m_pIndexBufferView->SizeInBytes = (UINT)indices.size() * sizeof(std::uint32_t);

		if (neverUpdate)
		{
			m_pIndexUploadBuffer.release();
			m_pVertexUploadBuffer.release();
		}

		m_vObjectParameters.emplace_back(ObjectDesc{ vertices.size(), 0, indices.size(), 0, 0 });
	}

	Point::Point(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		std::vector<FD3DW::VertexFrameWork> vertices;
		std::vector<std::uint32_t> indices;

		vertices.emplace_back(FD3DW::VertexFrameWork());
		(*vertices.rbegin()).Pos = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
		indices = { 0 };

		FD3DW::BufferManager::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), (UINT)vertices.size(), m_pVertexBuffer, m_pVertexUploadBuffer);

		m_pVertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		m_pVertexBufferView->BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
		m_pVertexBufferView->SizeInBytes = (UINT)vertices.size() * sizeof(FD3DW::VertexFrameWork);
		m_pVertexBufferView->StrideInBytes = sizeof(FD3DW::VertexFrameWork);

		FD3DW::BufferManager::CreateDefaultBuffer<std::uint32_t>(pDevice, pCommandList, indices.data(), (UINT)indices.size(), m_pIndexBuffer, m_pIndexUploadBuffer);

		m_pIndexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		m_pIndexBufferView->BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
		m_pIndexBufferView->Format =  DXGI_FORMAT_R32_UINT;
		m_pIndexBufferView->SizeInBytes = (UINT)indices.size() * sizeof(std::uint32_t);

		if (neverUpdate)
		{
			m_pIndexUploadBuffer.release();
			m_pVertexUploadBuffer.release();
		}

		m_vObjectParameters.emplace_back(ObjectDesc{ vertices.size(), 0, indices.size(), 0, 0 });
	}

}