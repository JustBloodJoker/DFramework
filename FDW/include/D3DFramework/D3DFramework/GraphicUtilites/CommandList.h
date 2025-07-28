#pragma once
#include "../pch.h"

namespace FD3DW
{


	class CommandList
	{
	public:
		static std::unique_ptr<CommandList> CreateList(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type);

	public:

		CommandList() = delete;
		~CommandList();

		CommandList(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

		void ResetList();
		void ResetList(ID3D12PipelineState* pPSO);

		bool TryCloseList();
		void CloseList();
		
		void ExecuteList(ID3D12CommandQueue* pCommandQueue); //NEED TO SYNCHRONIZE AFTER CALL METHOD

		ID3D12GraphicsCommandList4* const* GetAdressCommandList() const;
		ID3D12GraphicsCommandList4* GetPtrCommandList() const;

		/////////////////////////
		//		OPERATORS

		operator ID3D12GraphicsCommandList4* () const;

	private:

		wrl::ComPtr<ID3D12GraphicsCommandList4> m_pCommandList;
		wrl::ComPtr<ID3D12CommandAllocator> m_pListAllocator;

	};



}