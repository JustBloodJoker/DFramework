#include "../pch.h"
#include "CommandList.h"


namespace FDW
{


	CommandList::~CommandList()
	{
		pCommandList->Close();
	}

	CommandList::CommandList(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type)
	{
		HRESULT_ASSERT(pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(pListAllocator.GetAddressOf())),
			"Command allocator create error");

		HRESULT_ASSERT(pDevice->CreateCommandList(0, type, pListAllocator.Get(), nullptr, IID_PPV_ARGS(pCommandList.GetAddressOf())),
			"Command list create error");
	}

	void CommandList::ResetList()
	{
		ResetList(nullptr);
	}

	void CommandList::ResetList(ID3D12PipelineState* pPSO)
	{
		HRESULT_ASSERT(pListAllocator->Reset(), "Command allocator reset error");

		HRESULT_ASSERT(pCommandList->Reset(pListAllocator.Get(), pPSO), "Command list reset error");
	}

	bool CommandList::TryCloseList()
	{
		return FAILED(pCommandList->Close());
	}

	void CommandList::CloseList()
	{
		HRESULT_ASSERT(pCommandList->Close(), "COMMAND LIST CLOSE ERROR!");
	}

	void CommandList::ExecuteList(ID3D12CommandQueue* pCommandQueue)
	{
		CloseList();
		ID3D12CommandList* cmdLists[] = { pCommandList.Get() };
		pCommandQueue->ExecuteCommandLists(ARRAYSIZE(cmdLists), cmdLists);
	}

	ID3D12GraphicsCommandList* CommandList::GetPtrCommandList() const
	{
		return pCommandList.Get();
	}

	CommandList::operator ID3D12GraphicsCommandList* () const
	{
		return pCommandList.Get();
	}

	ID3D12GraphicsCommandList* const* CommandList::GetAdressCommandList() const
	{
		return pCommandList.GetAddressOf();
	}


}