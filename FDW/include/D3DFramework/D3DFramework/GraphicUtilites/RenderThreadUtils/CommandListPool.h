#pragma once

#include "../../pch.h"
#include "CommandRecipe.h"
#include "../CommandListTemplate.h"

namespace FD3DW {

    class CommandListPool {
    public:
        CommandListPool(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE type);

        std::unique_ptr<CommandListTemplate<ID3D12GraphicsCommandList>> BuildFromRecipe(const std::shared_ptr<ICommandRecipe>& recipe);

        void Recycle(std::unique_ptr<ICommandList> listBase);

    private:
        std::unique_ptr<CommandListTemplate<ID3D12GraphicsCommandList>> Acquire();

    private:
        ID3D12Device* m_pDevice = nullptr;
        D3D12_COMMAND_LIST_TYPE m_xType;

        std::mutex m_xMutex;
        std::vector<std::unique_ptr<CommandListTemplate<ID3D12GraphicsCommandList>>> m_vFree;
    };

}
