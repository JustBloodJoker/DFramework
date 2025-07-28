#include "../pch.h"
#include "CommandList.h"


namespace FD3DW
{
	std::unique_ptr<CommandList> CommandList::CreateList(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type)
	{
		return std::make_unique<CommandList>(pDevice, type);
	}

	CommandList::~CommandList()
	{
		m_pCommandList->Close();
	}

	CommandList::CommandList(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type)
	{
		HRESULT_ASSERT(pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(m_pListAllocator.GetAddressOf())),
			"Command allocator create error");

		HRESULT_ASSERT(pDevice->CreateCommandList(0, type, m_pListAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf())),
			"Command list create error");
	}

	void CommandList::ResetList()
	{
		ResetList(nullptr);
	}

	void CommandList::ResetList(ID3D12PipelineState* pPSO)
	{
		HRESULT_ASSERT(m_pListAllocator->Reset(), "Command allocator reset error");

		HRESULT_ASSERT(m_pCommandList->Reset(m_pListAllocator.Get(), pPSO), "Command list reset error");
	}

	bool CommandList::TryCloseList()
	{
		return FAILED(m_pCommandList->Close());
	}

	void CommandList::CloseList()
	{
		HRESULT_ASSERT(m_pCommandList->Close(), "COMMAND LIST CLOSE ERROR!");
	}

	void CommandList::ExecuteList(ID3D12CommandQueue* pCommandQueue)
	{
		CloseList();
		ID3D12CommandList* cmdLists[] = { m_pCommandList.Get() };
		pCommandQueue->ExecuteCommandLists(ARRAYSIZE(cmdLists), cmdLists);
	}

	ID3D12GraphicsCommandList4* CommandList::GetPtrCommandList() const
	{
		return m_pCommandList.Get();
	}

	CommandList::operator ID3D12GraphicsCommandList4* () const
	{
		return m_pCommandList.Get();
	}

	ID3D12GraphicsCommandList4* const* CommandList::GetAdressCommandList() const
	{
		return m_pCommandList.GetAddressOf();
	}


}