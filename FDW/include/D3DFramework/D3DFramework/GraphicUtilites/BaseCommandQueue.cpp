#include "BaseCommandQueue.h"

namespace FD3DW {


	BaseCommandQueue::BaseCommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags, INT priority, UINT nodeMask) {
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = flags;
		queueDesc.Priority = priority;
		queueDesc.NodeMask = nodeMask;
		queueDesc.Type = type;

		m_xType = type;

		HRESULT_ASSERT(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.ReleaseAndGetAddressOf())), "Command queue create error");
		HRESULT_ASSERT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf())), "Fence create error");
	}

	ID3D12CommandQueue* BaseCommandQueue::GetQueue() const
	{
		return m_pCommandQueue.Get();
	}

	ID3D12Fence* BaseCommandQueue::GetFence() const
	{
		return m_pFence.Get();
	}


	D3D12_COMMAND_LIST_TYPE BaseCommandQueue::GetType() const {
		return m_xType;
	}


}
