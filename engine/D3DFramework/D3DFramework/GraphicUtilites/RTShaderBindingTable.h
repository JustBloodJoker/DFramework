#pragma once

#include "../pch.h"
#include "RTPipelineObject.h"

namespace FD3DW {


	struct ShaderTable {
		wrl::ComPtr<ID3D12Resource> Buffer;
		uint32_t RecordSize = 0;
		uint32_t RecordCount = 0;

		D3D12_GPU_VIRTUAL_ADDRESS Address() const { return Buffer->GetGPUVirtualAddress(); }
		uint32_t SizeInBytes() const { return RecordSize * RecordCount; }
	};

	class RTShaderBindingTable {
	public:
		RTShaderBindingTable(RTPipelineObject* object);

		void InitSBT(ID3D12Device5* device);
		void UpdateLocalRootArgs(const std::wstring& entryName, const void* data, size_t size);

		D3D12_DISPATCH_RAYS_DESC* GetDispatchRaysDesc(uint32_t width, uint32_t height, uint32_t depth);

	private:
		void CreateSBTBuffers(ID3D12Device5* device);
		void CreateShaderTable(ID3D12Device5* device, ID3D12StateObjectProperties* props, const std::vector<std::wstring>& entries, ShaderTable& outTable);
		wrl::ComPtr<ID3D12Resource> CreateUploadBuffer(ID3D12Device5* device, const std::vector<uint8_t>& data);
		uint32_t Align(uint32_t size, uint32_t align);

	private:
		ShaderTable m_xRayGenTable;
		ShaderTable m_xHitGroupTable;
		ShaderTable m_xMissTable;

		std::unique_ptr<D3D12_DISPATCH_RAYS_DESC> m_pRaysDesc;
		RTPipelineObject* m_pPipeline;

		wrl::ComPtr<ID3D12Resource> m_pRayGenBuffer;
		wrl::ComPtr<ID3D12Resource> m_pMissBuffer;
		wrl::ComPtr<ID3D12Resource> m_pHitGroupBuffer;

		D3D12_DISPATCH_RAYS_DESC m_xDispatchDesc = {};

		uint32_t m_uShaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		uint32_t m_uRayGenRecordSize = 0;
		uint32_t m_uMissRecordSize = 0;
		uint32_t m_uHitGroupRecordSize = 0;

		std::unordered_map<std::wstring, std::vector<uint8_t>> m_mLocalRootArgs;
	};

}