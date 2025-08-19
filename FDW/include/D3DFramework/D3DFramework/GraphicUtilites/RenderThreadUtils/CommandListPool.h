#pragma once

#include "../../pch.h"
#include "CommandRecipe.h"
#include "..\CommandListTemplate.h"

namespace FD3DW
{
    class CommandListPool {
    public:
        CommandListPool(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE type);
        virtual ~CommandListPool() = default;
        
        std::unique_ptr<CommandListTemplate<ID3D12GraphicsCommandList>> BuildFromRecipe(const std::shared_ptr<ICommandRecipe>& recipe) {
            auto ret = CommandListTemplate<ID3D12GraphicsCommandList>::CreateList(m_pDevice, m_xType);

            recipe->Record(ret->GetPtrCommandList());

            ret->CloseList();
            return std::move(ret);
        }

    protected:
        ID3D12Device* m_pDevice = nullptr;
        D3D12_COMMAND_LIST_TYPE m_xType;
    };

}