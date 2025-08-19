#include "RenderThreadManager.h"

namespace FD3DW {
    
    static const UINT s_uInitialQueuesCountDefault = 1u;
    static const UINT s_uMaxQueuesCountDefault = 1u;
    static const UINT s_uLoadThresholdToSpawnDefault = 10u;

    void RenderThreadManager::CorrectConfig(RenderThreadManagerConfig& config)
    {
        for (auto& [type, perQueueConfig] : config.QueuesConfig) {
            perQueueConfig.InitialQueuesCount = std::max(perQueueConfig.InitialQueuesCount, 1u);
            perQueueConfig.MaxQueuesCount = std::max(perQueueConfig.MaxQueuesCount, 1u);

            if (perQueueConfig.LoadThresholdToSpawn==0) perQueueConfig.LoadThresholdToSpawn = 10u;
        }
    }

    void RenderThreadManager::Init(ID3D12Device* device, RenderThreadManagerConfig config) {
		m_xConfig = config;
		m_pDevice = device;
        
        for (const auto& [type, perQueueConfig] : m_xConfig.QueuesConfig) {
            auto initialVal = std::min(perQueueConfig.InitialQueuesCount, perQueueConfig.MaxQueuesCount);
            for (UINT i = 0; i < initialVal; ++i) {
                m_mQueues[type].emplace_back(std::make_unique<AsyncCommandQueue>(m_pDevice, type));
            }
        }

        m_xBuilder.Start();
	}

    void RenderThreadManager::Shutdown()
    {
        m_xBuilder.Stop();
        m_mQueues.clear();
    }

    std::shared_ptr<ExecutionHandle> RenderThreadManager::Submit(const std::shared_ptr<ICommandRecipe>& recipe)
    {
        auto handle = std::make_shared<ExecutionHandle>();
        EnqueueBuildAndSubmit({ recipe }, handle, false);
        return handle;
    }

    std::shared_ptr<ExecutionHandle> RenderThreadManager::SubmitChain(const std::vector<std::shared_ptr<ICommandRecipe>>& recipes)
    {
        auto handle = std::make_shared<ExecutionHandle>();
        EnqueueBuildAndSubmit(recipes, handle, true);
        return handle;
    }

    void RenderThreadManager::WaitIdle()
    {
        for (const auto& [type, queues] : m_mQueues) {
            for (const auto& queue : queues) {
                queue->WaitIdle();
            }
        }
    }


    void RenderThreadManager::WaitIdle(D3D12_COMMAND_LIST_TYPE type)
    {
        auto& queues = m_mQueues[type];
        for (const auto& queue : queues) {
            queue->WaitIdle();
        }
    }

    AsyncCommandQueue* RenderThreadManager::PickLeastLoaded(D3D12_COMMAND_LIST_TYPE type)
    {
        auto& arr = m_mQueues[type];
        auto& config = m_xConfig.QueuesConfig[type];
        if (arr.empty()) return nullptr;

        auto best = 0u;
        auto bestLoad = arr[0]->GetLoadApprox();

        auto allAboveThreshold = (bestLoad >= config.LoadThresholdToSpawn);

        for (UINT i = 1; i < arr.size(); ++i) 
        {
            auto l = arr[i]->GetLoadApprox();
            if (l < bestLoad) 
            {
                bestLoad = l;
                best = i;
            }

            if (l < config.LoadThresholdToSpawn)
            {
                allAboveThreshold = false;
            }
               
        }

        if (allAboveThreshold && arr.size() < config.MaxQueuesCount) 
        {
            auto q = std::make_unique<AsyncCommandQueue>(m_pDevice, type);
            auto qPtr = q.get();
            arr.push_back(std::move(q));
            return qPtr;
        }

        return arr[best].get();
    }

    void RenderThreadManager::EnqueueBuildAndSubmit(const std::vector<std::shared_ptr<ICommandRecipe>>& recipes, std::shared_ptr<ExecutionHandle> handle, bool forceSameQueue)
    {
        m_xBuilder.PostTask([this, recipes = recipes, handle = handle, forceSameQueue]() {

            if (recipes.empty()) { if (handle) handle->Bind(nullptr, 0); return; }

            CommandListPool direct(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
            CommandListPool compute(m_pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
            CommandListPool copy(m_pDevice, D3D12_COMMAND_LIST_TYPE_COPY);

            auto buildOne = [&](const std::shared_ptr<ICommandRecipe>& r) -> auto {
                switch (r->GetType()) {
                    case D3D12_COMMAND_LIST_TYPE_DIRECT:  return direct.BuildFromRecipe(r);
                    case D3D12_COMMAND_LIST_TYPE_COMPUTE: return compute.BuildFromRecipe(r);
                    case D3D12_COMMAND_LIST_TYPE_COPY:    return copy.BuildFromRecipe(r);
                    default: return direct.BuildFromRecipe(r);
                }
            };
            
            if (forceSameQueue) {
                AsyncCommandQueue* prevQueue = nullptr;
                for (const auto& r : recipes) {
                    auto q = PickLeastLoaded(r->GetType());

                    auto rec = buildOne(r);

                    auto singleBatch = std::make_shared<ClosedBatch>();
                    singleBatch->Handle = handle;
                    singleBatch->Lists.push_back(std::move(rec));

                    if (prevQueue && prevQueue != q) {
                        UINT64 fenceVal = prevQueue->SignalFence();
                        q->WaitFence(fenceVal, prevQueue);
                    }

                    q->Submit(singleBatch);
                    prevQueue = q;
                }
            }
            else {
                for (auto& r : recipes) {
                    auto q = PickLeastLoaded(r->GetType());

                    auto rec = buildOne(r);

                    auto batch = std::make_shared<ClosedBatch>();
                    batch->Handle = handle;

                    auto recPtr = rec->GetPtrDefaultCommandList();
                    batch->Lists.push_back(std::move(rec));

                    q->Submit(batch);
                }
            }
            });
    }

}



