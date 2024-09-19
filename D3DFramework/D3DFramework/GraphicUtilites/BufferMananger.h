#pragma once
#include "../pch.h"


namespace FD3DW
{

	template<typename BUFFER_STRUCTURE_DESC_TYPE>
	class UploadBuffer
	{
	
		using type = BUFFER_STRUCTURE_DESC_TYPE;

	public:

		static UINT CalculateConstantBufferSize(UINT bs);

		UploadBuffer(ID3D12Device* pDevice, UINT elementNum, bool isCBBuffer);
		~UploadBuffer();

		D3D12_GPU_VIRTUAL_ADDRESS GetGPULocation(const size_t index) const;

		ID3D12Resource* GetResource() const;
		void CpyData(int index, const BUFFER_STRUCTURE_DESC_TYPE& data);

		UINT GetDataSize() const;

	private:

		BYTE* pData;
		UINT dataSize;
		wrl::ComPtr<ID3D12Resource> pUploadBuffer;

	};



	class BufferMananger
	{

	public:

		template<typename BUFFER_STRUCTURE_TYPE>
		static bool CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pInitData, UINT elementCount, wrl::ComPtr<ID3D12Resource>& dstBuffer, std::unique_ptr<UploadBuffer<BUFFER_STRUCTURE_TYPE>>& pUploadBuffer);

		
	private:



	};


	template<typename BUFFER_STRUCTURE_DESC_TYPE>
	inline UINT UploadBuffer<BUFFER_STRUCTURE_DESC_TYPE>::CalculateConstantBufferSize(UINT bs)
	{
		return (bs + 255) & ~255;
	}

	template<typename BUFFER_STRUCTURE_DESC_TYPE>
	inline UploadBuffer<BUFFER_STRUCTURE_DESC_TYPE>::UploadBuffer(ID3D12Device* pDevice, UINT elementNum, bool isCBBuffer)
	{
		isCBBuffer ? dataSize = UploadBuffer<BUFFER_STRUCTURE_DESC_TYPE>::CalculateConstantBufferSize(sizeof(BUFFER_STRUCTURE_DESC_TYPE)) : dataSize = sizeof(BUFFER_STRUCTURE_DESC_TYPE);

		if (pUploadBuffer)
			pUploadBuffer->Release();

		HRESULT_ASSERT(pDevice->CreateCommittedResource(
			&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
			D3D12_HEAP_FLAG_NONE,
			&keep(CD3DX12_RESOURCE_DESC::Buffer(elementNum * dataSize)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pUploadBuffer.GetAddressOf())),
			"upload buffer create error");

		HRESULT_ASSERT(pUploadBuffer->Map(0, nullptr,
			reinterpret_cast<void**>(&pData)), "Map upload buffer error");
	}

	template<typename BUFFER_STRUCTURE_DESC_TYPE>
	inline UploadBuffer<BUFFER_STRUCTURE_DESC_TYPE>::~UploadBuffer()
	{
		if (pUploadBuffer)
			pUploadBuffer->Unmap(0, nullptr);

		pData = nullptr;
	}

	template<typename BUFFER_STRUCTURE_DESC_TYPE>
	inline D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer<BUFFER_STRUCTURE_DESC_TYPE>::GetGPULocation(const size_t index) const
	{
		return pUploadBuffer->GetGPUVirtualAddress() + dataSize * index;
	}

	template<typename BUFFER_STRUCTURE_DESC_TYPE>
	inline ID3D12Resource* UploadBuffer<BUFFER_STRUCTURE_DESC_TYPE>::GetResource() const
	{
		return pUploadBuffer.Get();
	}


	template<typename BUFFER_STRUCTURE_DESC_TYPE>
	inline void UploadBuffer<BUFFER_STRUCTURE_DESC_TYPE>::CpyData(int index, const BUFFER_STRUCTURE_DESC_TYPE& data)
	{
		memcpy(&pData[index * dataSize], &data, sizeof(BUFFER_STRUCTURE_DESC_TYPE));
	}

	template<typename BUFFER_STRUCTURE_DESC_TYPE>
	inline UINT UploadBuffer<BUFFER_STRUCTURE_DESC_TYPE>::GetDataSize() const
	{
		return dataSize;
	}


	template<typename BUFFER_STRUCTURE_TYPE>
	inline bool BufferMananger::CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pInitData, UINT elementCount, wrl::ComPtr<ID3D12Resource>& dstBuffer, std::unique_ptr<UploadBuffer<BUFFER_STRUCTURE_TYPE>>& pUploadBuffer)
	{
		if (dstBuffer)
			dstBuffer->Release();

		HRESULT_ASSERT(pDevice->CreateCommittedResource(&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
			D3D12_HEAP_FLAG_NONE,
			&keep(CD3DX12_RESOURCE_DESC::Buffer(elementCount * sizeof(BUFFER_STRUCTURE_TYPE))),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(dstBuffer.GetAddressOf())),
			"Result Default Buffer create error");

		if (pUploadBuffer)
			pUploadBuffer.release();

		pUploadBuffer = std::make_unique<UploadBuffer<BUFFER_STRUCTURE_TYPE>>(pDevice, elementCount, false);
		
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = pInitData;
		subResourceData.RowPitch = elementCount * sizeof(BUFFER_STRUCTURE_TYPE);
		subResourceData.SlicePitch = subResourceData.RowPitch;


		pCommandList->ResourceBarrier(1,
			&keep(CD3DX12_RESOURCE_BARRIER::Transition(dstBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_COPY_DEST)));

		UpdateSubresources<1>(pCommandList,
			dstBuffer.Get(),
			pUploadBuffer->GetResource(),
			0, 0, 1, &subResourceData);

		pCommandList->ResourceBarrier(1,
			&keep(CD3DX12_RESOURCE_BARRIER::Transition(dstBuffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_GENERIC_READ)));

		return dstBuffer;
	}
}