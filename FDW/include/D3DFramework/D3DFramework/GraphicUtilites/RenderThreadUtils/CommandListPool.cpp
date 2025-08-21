#include "CommandListPool.h"

namespace FD3DW {
	
	CommandListPool::CommandListPool(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE type) : m_pDevice(dev), m_xType(type) {}

	std::unique_ptr<CommandListTemplate<ID3D12GraphicsCommandList>> CommandListPool::BuildFromRecipe(const std::shared_ptr<ICommandRecipe>& recipe)
    {
        auto cmd = Acquire();
        recipe->Record(cmd->GetPtrCommandList());
        cmd->CloseList();
        return cmd;
    }

    void CommandListPool::Recycle(std::unique_ptr<ICommandList> listBase)
    {
        auto* raw = listBase.release();
        auto* typed = dynamic_cast<CommandListTemplate<ID3D12GraphicsCommandList>*>(raw);
        if (!typed) { 
            delete raw;
            return;
        }

        std::unique_ptr<CommandListTemplate<ID3D12GraphicsCommandList>> ptr(typed);

        std::lock_guard<std::mutex> lock(m_xMutex);
        m_vFree.push_back(std::move(ptr));
    }

    std::unique_ptr<CommandListTemplate<ID3D12GraphicsCommandList>> CommandListPool::Acquire()
    {
        std::lock_guard<std::mutex> lock(m_xMutex);
        if (!m_vFree.empty()) {
            auto ret = std::move(m_vFree.back());
            m_vFree.pop_back();
            ret->ResetList();
            return ret;
        }
        return CommandListTemplate<ID3D12GraphicsCommandList>::CreateList(m_pDevice, m_xType);
    }
}
