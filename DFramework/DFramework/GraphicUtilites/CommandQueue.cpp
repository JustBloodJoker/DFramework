#include "../pch.h"
#include "CommandQueue.h"

namespace FDW
{


	CommandQueue::CommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags, INT priority, UINT nodeMask)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = flags;
		queueDesc.Priority = priority;
		queueDesc.NodeMask = nodeMask;
		queueDesc.Type = type;

		HRESULT_ASSERT(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(pCommandQueue.ReleaseAndGetAddressOf())), "Command queue create error");
		HRESULT_ASSERT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(pFence.ReleaseAndGetAddressOf())), "Fence create error");
	}

	void CommandQueue::ExecuteQueue(bool synch)
	{
		
		for (const auto& el : bindedLists)
		{
			el->TryCloseList();
			executedlists.push_back(el->GetPtrCommandList());
		}

		pCommandQueue->ExecuteCommandLists(executedlists.size(), executedlists.data());

		executedlists.clear();

		synch ? FlushQueue() : (void)0 ;
	}

	void CommandQueue::BindCommandList(CommandList* pCommandList)
	{
		auto ptr = std::find(bindedLists.begin(), bindedLists.end(), pCommandList);
		if (ptr == bindedLists.end())
		{
			bindedLists.push_back(pCommandList);
			CONSOLE_MESSAGE("Command list is binding to Command Queue");
		}
		else
		{
			CONSOLE_MESSAGE("Command list is already binded");
		}
	}

	void CommandQueue::UnbindCommandList(CommandList* pCommandList)
	{
		auto ptr = std::find(bindedLists.begin(), bindedLists.end(), pCommandList);
		if (ptr != bindedLists.end())
		{
			bindedLists.erase(ptr);
			CONSOLE_MESSAGE("Command list is unbinding from Command Queue");
		}
		else
		{
			CONSOLE_MESSAGE("Can't find command list in command queue");
		}
	}

	ID3D12CommandQueue* CommandQueue::GetQueue() const
	{
		return pCommandQueue.Get();
	}

	void CommandQueue::FlushQueue()
	{
		auto nextFence = pFence->GetCompletedValue() + 1;
		HRESULT_ASSERT(pCommandQueue->Signal(pFence.Get(),
			nextFence), "Fence signal error");

		if (pFence->GetCompletedValue() < nextFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, L"", false, EVENT_ALL_ACCESS);
			HRESULT_ASSERT(pFence->SetEventOnCompletion(nextFence, eventHandle), "Fence set event error");
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}


}