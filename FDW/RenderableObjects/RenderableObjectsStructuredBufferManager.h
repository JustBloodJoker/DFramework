#pragma once

#include <pch.h>
#include <WinWindow/Utils/CreativeSingleton.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <RenderableObjects/MeshMatricesStructure.h>
#include <RenderableObjects/BaseRenderableObject.h>

template<typename T>
class RenderableObjectsStructuredBufferManager : public FDWWIN::CreativeSingleton<RenderableObjectsStructuredBufferManager<T>> {
public:
	RenderableObjectsStructuredBufferManager() = default;
	virtual ~RenderableObjectsStructuredBufferManager() = default;

public:
	void Init(UINT precacheSize, ID3D12Device* device)
	{
		if (m_bIsInited) return;

		m_pBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<T>(device, precacheSize, true);

		m_bIsInited = true;
	}

	UINT GenerateIndexForBuffer(UINT count) {
		if (count == 0) return UINT_MAX;

		UINT consecutiveFree = 0;
		UINT startIdx = 0;

		auto topIdx = m_mDataMap.empty() ? -1 : m_mDataMap.rbegin()->first;
		for (UINT idx = 0; idx < topIdx; ++idx) {
			if (m_mDataMap.find(idx) == m_mDataMap.end()) {
				if (consecutiveFree == 0)
					startIdx = idx;

				++consecutiveFree;

				if (consecutiveFree == count)
					return startIdx;
			}
			else {
				consecutiveFree = 0;
			}
		}
		return topIdx + 1;
	}

	UINT GenerateIndexForBuffer() {
		auto topIdx = m_mDataMap.empty() ? -1 : m_mDataMap.rbegin()->first;
		for (UINT idx = 0; idx < topIdx; ++idx)
		{
			if (m_mDataMap.find(idx) == m_mDataMap.end())
			{
				return idx;
			}
		}

		return topIdx +1;
	}

	void RemoveDataFromIndex(UINT idx) {
		m_mDataMap.erase(idx);
	}
	
	void UploadDataToIndex(UINT idx, T data) {
		m_mDataMap[idx] = data;
		m_vChangedData.push_back(idx);
	}
	
	void UploadDataRange(UINT startIdx, const std::vector<T>& dataVec) {
		if (dataVec.empty()) return;

		for (size_t i = 0; i < dataVec.size(); ++i) {
			UINT idx = startIdx + static_cast<UINT>(i);
			m_mDataMap[idx] = dataVec[i];
			m_vChangedData.push_back(idx);
		}
	}

	void SetBufferState(ID3D12GraphicsCommandList* list, D3D12_RESOURCE_STATES state) {
		m_pBuffer->ResourceBarrierChange(list, 1, state);
	}

	void BeforeRender(ID3D12GraphicsCommandList* list, ID3D12Device* device) {
		if (m_vChangedData.empty()) return;
		
		std::sort(m_vChangedData.begin(), m_vChangedData.end());
		m_vChangedData.erase(std::unique(m_vChangedData.begin(), m_vChangedData.end()), m_vChangedData.end());

		UINT rangeStart = m_vChangedData[0];
		UINT rangeEnd = rangeStart;

		for (size_t i = 1; i <= m_vChangedData.size(); ++i)
		{
			bool isLast = (i == m_vChangedData.size());
			bool isGap = !isLast && (m_vChangedData[i] != rangeEnd + 1);

			if (isLast || isGap)
			{
				const UINT numElements = rangeEnd - rangeStart + 1;
				std::vector<T> tempData;
				tempData.reserve(numElements);

				for (UINT idx = rangeStart; idx <= rangeEnd; ++idx)
				{
					auto it = m_mDataMap.find(idx);
					if (it != m_mDataMap.end())
						tempData.push_back(it->second);
					else
						tempData.emplace_back();
				}

				m_pBuffer->UploadRegion(device, list, tempData.data(), rangeStart, numElements);

				if (!isLast)
				{
					rangeStart = m_vChangedData[i];
					rangeEnd = rangeStart;
				}
			}
			else
			{
				rangeEnd = m_vChangedData[i];
			}
		}
		m_vChangedData.clear();
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUBufferAddress() {
		return m_pBuffer->GetResource()->GetGPUVirtualAddress();
	}

protected:
	std::vector<UINT> m_vChangedData;
	std::map<UINT, T> m_mDataMap;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pBuffer;
	bool m_bIsInited = false;

};




