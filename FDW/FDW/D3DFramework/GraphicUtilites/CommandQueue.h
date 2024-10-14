#pragma once
#include "../pch.h"
#include "CommandList.h"


namespace FD3DW
{

	class CommandQueue
	{

	public:

		CommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE, INT priority = 0, UINT nodeMask = 0);
		CommandQueue()=delete;
		~CommandQueue()=default;


		void ExecuteQueue(bool synch);

		void BindCommandList(CommandList* pCommandList);
		void UnbindCommandList(CommandList* pCommandList);

		ID3D12CommandQueue* GetQueue() const;

		void FlushQueue();

	private:


		std::vector<ID3D12CommandList*> executedlists;
		std::vector<CommandList*> bindedLists;
		wrl::ComPtr<ID3D12CommandQueue> pCommandQueue;
		wrl::ComPtr<ID3D12Fence> pFence;
	};

}