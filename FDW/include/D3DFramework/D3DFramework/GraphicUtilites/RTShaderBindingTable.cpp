#include "RTShaderBindingTable.h"


namespace FD3DW {

	RTShaderBindingTable::RTShaderBindingTable(RTPipelineObject* object)
		: m_pPipeline(object) {
	}

	void RTShaderBindingTable::UpdateLocalRootArgs(const std::wstring& entryName, const void* data, size_t size) {
		auto& vec = m_mLocalRootArgs[entryName];
		vec.resize(size);
		memcpy(vec.data(), data, size);
	}

	void RTShaderBindingTable::InitSBT(ID3D12Device5* device) {
		CreateSBTBuffers(device);
	}

	void RTShaderBindingTable::CreateSBTBuffers(ID3D12Device5* device) {
        auto pStateObject = m_pPipeline->GetStateObject();
        ID3D12StateObjectProperties* props;
        hr = pStateObject->QueryInterface(IID_PPV_ARGS(&props));

        const auto& rayGenEntry = m_pPipeline->GetRayGenEntry();
        const auto& missEntries = m_pPipeline->GetMissEntries();
        const auto& hitGroupEntries = m_pPipeline->GetHitGroupEntries();

        CreateShaderTable(device, props, { rayGenEntry }, m_xRayGenTable);
        CreateShaderTable(device, props, missEntries, m_xMissTable);
        CreateShaderTable(device, props, hitGroupEntries, m_xHitGroupTable);
	}

    D3D12_DISPATCH_RAYS_DESC* RTShaderBindingTable::GetDispatchRaysDesc(uint32_t width, uint32_t height, uint32_t depth) {
        if(!m_pRaysDesc) m_pRaysDesc = std::make_unique<D3D12_DISPATCH_RAYS_DESC>();

        m_pRaysDesc->RayGenerationShaderRecord = { m_xRayGenTable.Address(), m_xRayGenTable.RecordSize };
        m_pRaysDesc->MissShaderTable = { m_xMissTable.Address(), m_xMissTable.SizeInBytes(), m_xMissTable.RecordSize };
        m_pRaysDesc->HitGroupTable = { m_xHitGroupTable.Address(), m_xHitGroupTable.SizeInBytes(), m_xHitGroupTable.RecordSize };

        m_pRaysDesc->Width = width;
        m_pRaysDesc->Height = height;
        m_pRaysDesc->Depth = depth;

        return m_pRaysDesc.get();
    }
    void RTShaderBindingTable::CreateShaderTable(ID3D12Device5* device, ID3D12StateObjectProperties* props,const std::vector<std::wstring>& entries, ShaderTable& outTable)
    {
        assert(!entries.empty());

        outTable.RecordCount = (uint32_t)entries.size();
        outTable.RecordSize = 0;

        std::vector<uint8_t> allRecords;

        for (const auto& entry : entries) {
            void* id = props->GetShaderIdentifier(entry.c_str());
            assert(id);

            const auto& args = m_mLocalRootArgs[entry];
            uint32_t recordSize = Align(m_uShaderIdSize + (uint32_t)args.size(), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

            if (outTable.RecordSize == 0)
                outTable.RecordSize = recordSize;
            else
                assert(outTable.RecordSize == recordSize);

            std::vector<uint8_t> record(recordSize);
            memcpy(record.data(), id, m_uShaderIdSize);
            if (!args.empty())
                memcpy(record.data() + m_uShaderIdSize, args.data(), args.size());

            allRecords.insert(allRecords.end(), record.begin(), record.end());
        }

        outTable.Buffer = CreateUploadBuffer(device, allRecords);
    }

    wrl::ComPtr<ID3D12Resource> RTShaderBindingTable::CreateUploadBuffer(ID3D12Device5* device, const std::vector<uint8_t>& data) {
		wrl::ComPtr<ID3D12Resource> buffer;
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(data.size());
	    device->CreateCommittedResource(
			&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&buffer)
		);

		void* mappedData = nullptr;
		buffer->Map(0, nullptr, &mappedData);
		memcpy(mappedData, data.data(), data.size());
		buffer->Unmap(0, nullptr);

		return buffer;
	}

	uint32_t RTShaderBindingTable::Align(uint32_t size, uint32_t align) {
		return (size + align - 1) & ~(align - 1);
	}



}
