#pragma once
#include "../pch.h"
#include "ICommandList.h"

namespace FD3DW
{
	
    template<typename D3DCommandListType, typename = std::enable_if_t<std::is_base_of_v<ID3D12GraphicsCommandList, D3DCommandListType>>>
    class CommandListTemplate : public ICommandList
    {
    public:
        static std::unique_ptr<CommandListTemplate<D3DCommandListType>> CreateList(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type) {
            return std::make_unique<CommandListTemplate<D3DCommandListType>>(pDevice, type);
        }

    public:
        CommandListTemplate() = default;
        virtual ~CommandListTemplate() {
            TryCloseList();
        }

        CommandListTemplate(ID3D12Device* pDevice, const D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) {
            m_xType = type;
            HRESULT_ASSERT(pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(m_pListAllocator.GetAddressOf())),
                "Command allocator create error");

            HRESULT_ASSERT(pDevice->CreateCommandList(0, type, m_pListAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf())),
                "Command list create error");
        }

        void ResetList() {
            ResetList(nullptr);
        }

        void ResetList(ID3D12PipelineState* pPSO) {
            HRESULT_ASSERT(m_pListAllocator->Reset(), "Command allocator reset error");

            HRESULT_ASSERT(m_pCommandList->Reset(m_pListAllocator.Get(), pPSO), "Command list reset error");
        }

        virtual bool TryCloseList() override {
            return FAILED(m_pCommandList->Close());
        }

        virtual void CloseList() override {
            HRESULT_ASSERT(m_pCommandList->Close(), "COMMAND LIST CLOSE ERROR!");
        }

        // NEED TO SYNCHRONIZE AFTER CALL METHOD
        void ExecuteList(ID3D12CommandQueue* pCommandQueue) {
            TryCloseList();
            ID3D12CommandList* cmdLists[] = { m_pCommandList.Get() };
            pCommandQueue->ExecuteCommandLists(ARRAYSIZE(cmdLists), cmdLists);
        }

        virtual ID3D12CommandList* GetPtrDefaultCommandList() const override {
            return GetPtrCommandList();
        }

        virtual D3D12_COMMAND_LIST_TYPE GetType() const override {
            return m_xType;
        }

        D3DCommandListType* const* GetAdressCommandList() const {
            return m_pCommandList.GetAddressOf();

        }
        D3DCommandListType* GetPtrCommandList() const {
            return m_pCommandList.Get();
        }
        
        operator D3DCommandListType* () const {
            return m_pCommandList.Get();
        }

    private:
        wrl::ComPtr<D3DCommandListType> m_pCommandList;
        wrl::ComPtr<ID3D12CommandAllocator> m_pListAllocator;
        D3D12_COMMAND_LIST_TYPE m_xType;
    };




}