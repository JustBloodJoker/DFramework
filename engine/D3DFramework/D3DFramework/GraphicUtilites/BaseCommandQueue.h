#pragma once

#include "../pch.h"

namespace FD3DW {

class BaseCommandQueue {
public:
	BaseCommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE, INT priority = 0, UINT nodeMask = 0);
	virtual ~BaseCommandQueue() = default;


	ID3D12CommandQueue* GetQueue() const;
	ID3D12Fence* GetFence() const;
	D3D12_COMMAND_LIST_TYPE GetType() const;


protected:


	wrl::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	wrl::ComPtr<ID3D12Fence> m_pFence;
	D3D12_COMMAND_LIST_TYPE m_xType;

};


}
