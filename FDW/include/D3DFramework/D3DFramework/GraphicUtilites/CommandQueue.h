#pragma once
#include "../pch.h"
#include "ICommandList.h"


namespace FD3DW
{

	class CommandQueue
	{

	public:
		static std::unique_ptr<CommandQueue> CreateQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE, size_t priority = 0, size_t nodeMask = 0);

	public:

		CommandQueue(ID3D12Device* pDevice, ICommandList* pCommandList);
		CommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE, INT priority = 0, UINT nodeMask = 0);
		CommandQueue()=delete;
		~CommandQueue()=default;


		void ExecuteQueue(bool synch);

		void BindCommandList(ICommandList* pCommandList);
		void UnbindCommandList(ICommandList* pCommandList);

		ID3D12CommandQueue* GetQueue() const;

		void FlushQueue();

	private:


		std::vector<ID3D12CommandList*> m_vExecutedlists;
		std::vector<ICommandList*> m_vBindedLists;
		wrl::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
		wrl::ComPtr<ID3D12Fence> m_pFence;
	};

}