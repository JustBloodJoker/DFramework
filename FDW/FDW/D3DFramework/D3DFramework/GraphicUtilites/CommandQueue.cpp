#include "../pch.h"
#include "CommandQueue.h"

namespace FD3DW
{


	CommandQueue::CommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags, INT priority, UINT nodeMask)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = flags;
		queueDesc.Priority = priority;
		queueDesc.NodeMask = nodeMask;
		queueDesc.Type = type;
		
		HRESULT_ASSERT(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.ReleaseAndGetAddressOf())), "Command queue create error");
		HRESULT_ASSERT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf())), "Fence create error");
	}

	void CommandQueue::ExecuteQueue(bool synch)
	{
		
		for (const auto& el : m_vBindedLists)
		{
			el->TryCloseList();
			m_vExecutedlists.push_back(el->GetPtrCommandList());
		}

		m_pCommandQueue->ExecuteCommandLists((UINT)m_vExecutedlists.size(), m_vExecutedlists.data());

		m_vExecutedlists.clear();

		synch ? FlushQueue() : (void)0 ;
	}

	void CommandQueue::BindCommandList(CommandList* pCommandList)
	{
		auto ptr = std::find(m_vBindedLists.begin(), m_vBindedLists.end(), pCommandList);
		if (ptr == m_vBindedLists.end())
		{
			m_vBindedLists.push_back(pCommandList);
			CONSOLE_MESSAGE("Command list is binding to Command Queue");
		}
		else
		{
			CONSOLE_MESSAGE("Command list is already binded");
		}
	}

	void CommandQueue::UnbindCommandList(CommandList* pCommandList)
	{
		auto ptr = std::find(m_vBindedLists.begin(), m_vBindedLists.end(), pCommandList);
		if (ptr != m_vBindedLists.end())
		{
			m_vBindedLists.erase(ptr);
			CONSOLE_MESSAGE("Command list is unbinding from Command Queue");
		}
		else
		{
			CONSOLE_MESSAGE("Can't find command list in command queue");
		}
	}

	ID3D12CommandQueue* CommandQueue::GetQueue() const
	{
		return m_pCommandQueue.Get();
	}

	void CommandQueue::FlushQueue()
	{
		auto nextFence = m_pFence->GetCompletedValue() + 1;
		HRESULT_ASSERT(m_pCommandQueue->Signal(m_pFence.Get(),
			nextFence), "Fence signal error");

		if (m_pFence->GetCompletedValue() < nextFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, L"", false, EVENT_ALL_ACCESS);
			HRESULT_ASSERT(m_pFence->SetEventOnCompletion(nextFence, eventHandle), "Fence set event error");
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}


}