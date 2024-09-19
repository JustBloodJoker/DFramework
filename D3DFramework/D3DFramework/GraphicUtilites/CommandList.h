#pragma once
#include "../pch.h"

namespace FD3DW
{


	class CommandList
	{

	public:

		CommandList() = delete;
		~CommandList();

		CommandList(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

		void ResetList();
		void ResetList(ID3D12PipelineState* pPSO);

		bool TryCloseList();
		void CloseList();
		
		void ExecuteList(ID3D12CommandQueue* pCommandQueue); //NEED TO SYNCHRONIZE AFTER CALL METHOD

		ID3D12GraphicsCommandList* const* GetAdressCommandList() const;
		ID3D12GraphicsCommandList* GetPtrCommandList() const;

		/////////////////////////
		//		OPERATORS

		operator ID3D12GraphicsCommandList* () const;

	private:

		wrl::ComPtr<ID3D12GraphicsCommandList> pCommandList;
		wrl::ComPtr<ID3D12CommandAllocator> pListAllocator;

	};



}