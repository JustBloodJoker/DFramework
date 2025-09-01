#pragma once

#include "../../pch.h"
#include "AsyncCommandQueue.h"
#include "CommandListPool.h"
#include "CommandRecipe.h"

namespace FD3DW {

    class RenderThreadManager {
    public:
        RenderThreadManager() = default;
        virtual ~RenderThreadManager() = default;

        std::shared_ptr<ExecutionHandle> CreateWaitHandle(D3D12_COMMAND_LIST_TYPE type);

        void WaitForCurrentCommandGPU();
        void WaitForCurrentCommandGPU(D3D12_COMMAND_LIST_TYPE type);

        void WaitIdle();

        void Init(ID3D12Device* device);
        void Shutdown();

        AsyncCommandQueue* GetQueue(D3D12_COMMAND_LIST_TYPE type);

        std::shared_ptr<ExecutionHandle> Submit(const std::shared_ptr<ICommandRecipe>& recipe, std::vector<std::shared_ptr<ExecutionHandle>> dependencies = {});
        std::shared_ptr<ExecutionHandle> SubmitLambda(std::function<void()> func, std::vector<std::shared_ptr<ExecutionHandle>> dependencies = {});
        std::shared_ptr<ExecutionHandle> SubmitChain(const std::vector<std::shared_ptr<ICommandRecipe>>& recipes, std::vector<std::shared_ptr<ExecutionHandle>> dependencies = {});

        void GarbageCollectAll();

    private:
        AsyncCommandQueue* PickCommandQueue(D3D12_COMMAND_LIST_TYPE type);

    private:
        ID3D12Device* m_pDevice = nullptr;
        FDWWIN::WorkerThread m_xRenderThread;

        std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<AsyncCommandQueue>> m_mQueues;
        std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandListPool>>   m_mPools;
    };


}