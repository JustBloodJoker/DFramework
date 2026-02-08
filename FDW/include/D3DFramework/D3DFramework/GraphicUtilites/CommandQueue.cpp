#include "../pch.h"
#include "CommandQueue.h"

namespace FD3DW
{
	std::unique_ptr<CommandQueue> CommandQueue::CreateQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags, INT priority, UINT nodeMask)
	{
		return std::make_unique<CommandQueue>(pDevice, type, flags, priority, nodeMask);
	}
	CommandQueue::CommandQueue(ID3D12Device* pDevice, ICommandList* pCommandList) : CommandQueue(pDevice, pCommandList->GetType()) {
		BindCommandList(pCommandList);
	}

	CommandQueue::CommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags, INT priority, UINT nodeMask) 
		: BaseCommandQueue(pDevice, type, flags, priority, nodeMask){}

	void CommandQueue::ExecuteQueue(bool synch)
	{
		for (const auto& el : m_vBindedLists)
		{
			auto close = el->TryCloseList();
			m_vExecutedlists.push_back(el->GetPtrDefaultCommandList());
		}

		m_pCommandQueue->ExecuteCommandLists((UINT)m_vExecutedlists.size(), m_vExecutedlists.data());

		m_vExecutedlists.clear();

		synch ? FlushQueue() : (void)0 ;
	}

	void CommandQueue::BindCommandList(ICommandList* pCommandList)
	{
		auto ptr = std::find(m_vBindedLists.begin(), m_vBindedLists.end(), pCommandList);
		if (ptr == m_vBindedLists.end())
		{
			m_vBindedLists.push_back(pCommandList);
			CONSOLE_MESSAGE("Command list is binded to Command Queue");
		}
		else
		{
			CONSOLE_MESSAGE("Command list is already binded");
		}
	}

	void CommandQueue::UnbindCommandList(ICommandList* pCommandList)
	{
		auto ptr = std::find(m_vBindedLists.begin(), m_vBindedLists.end(), pCommandList);
		if (ptr != m_vBindedLists.end())
		{
			m_vBindedLists.erase(ptr);
			CONSOLE_MESSAGE("Command list is unbinded from Command Queue");
		}
		else
		{
			CONSOLE_MESSAGE("Can't find command list in command queue");
		}
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