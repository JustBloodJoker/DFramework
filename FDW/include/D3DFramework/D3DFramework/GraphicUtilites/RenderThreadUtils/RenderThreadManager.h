#pragma once

#include "../../pch.h"
#include "AsyncCommandQueue.h"
#include "CommandListPool.h"
#include "CommandRecipe.h"

namespace FD3DW {
   
    struct PerTypeQueueConfig {
        UINT InitialQueuesCount = 1;
        UINT MaxQueuesCount = 1;
        UINT LoadThresholdToSpawn = 10;
    };

    struct RenderThreadManagerConfig {
        std::map<D3D12_COMMAND_LIST_TYPE, PerTypeQueueConfig> QueuesConfig;
    };



    class RenderThreadManager
    {

    public:
        static void CorrectConfig(RenderThreadManagerConfig& config);

    public:
        void WaitIdle();
        void WaitIdle(D3D12_COMMAND_LIST_TYPE type);

        void Init(ID3D12Device* device, RenderThreadManagerConfig config);
        void Shutdown();

        std::shared_ptr<ExecutionHandle> Submit(const std::shared_ptr<ICommandRecipe>& recipe);
        std::shared_ptr<ExecutionHandle> SubmitChain(const std::vector<std::shared_ptr<ICommandRecipe>>& recipes);

    private:
        AsyncCommandQueue* PickLeastLoaded(D3D12_COMMAND_LIST_TYPE type);
        void EnqueueBuildAndSubmit(const std::vector<std::shared_ptr<ICommandRecipe>>& recipes, std::shared_ptr<ExecutionHandle> handle, bool forceSameQueue);
        


    private:
        ID3D12Device* m_pDevice = nullptr;

        std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::vector<std::unique_ptr<AsyncCommandQueue>>> m_mQueues;
        FDWWIN::WorkerThread m_xBuilder;
        RenderThreadManagerConfig m_xConfig;
    };

}