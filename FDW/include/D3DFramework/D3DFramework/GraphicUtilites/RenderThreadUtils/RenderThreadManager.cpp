#include "RenderThreadManager.h"

namespace FD3DW {

    void RenderThreadManager::Init(ID3D12Device* device) {
        m_pDevice = device;

        m_mQueues[D3D12_COMMAND_LIST_TYPE_DIRECT] = std::make_unique<AsyncCommandQueue>(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_mQueues[D3D12_COMMAND_LIST_TYPE_COMPUTE] = std::make_unique<AsyncCommandQueue>(m_pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
        m_mQueues[D3D12_COMMAND_LIST_TYPE_COPY] = std::make_unique<AsyncCommandQueue>(m_pDevice, D3D12_COMMAND_LIST_TYPE_COPY);

        m_mPools[D3D12_COMMAND_LIST_TYPE_DIRECT] = std::make_unique<CommandListPool>(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_mPools[D3D12_COMMAND_LIST_TYPE_COMPUTE] = std::make_unique<CommandListPool>(m_pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
        m_mPools[D3D12_COMMAND_LIST_TYPE_COPY] = std::make_unique<CommandListPool>(m_pDevice, D3D12_COMMAND_LIST_TYPE_COPY);

        m_xCommandsBuilder.Init(m_iMaxThreadsCount);
        m_xRenderThread.Start();
    }

    void RenderThreadManager::Shutdown() {
        WaitIdle();

        m_xRenderThread.Stop();

        GarbageCollectAll();

        m_mPools.clear();
        m_mQueues.clear();
    }

    AsyncCommandQueue* RenderThreadManager::GetQueue(D3D12_COMMAND_LIST_TYPE type) {
        return m_mQueues.contains(type) ? m_mQueues[type].get() : nullptr;
    }

    AsyncCommandQueue* RenderThreadManager::PickCommandQueue(D3D12_COMMAND_LIST_TYPE type) {
        return GetQueue(type);
    }

    std::shared_ptr<ExecutionHandle> RenderThreadManager::CreateWaitHandle(D3D12_COMMAND_LIST_TYPE type) {
        if (m_mQueues.contains(type)) return m_mQueues[type]->CreateReservedExecutionHandle();

        auto h = std::make_shared<ExecutionHandle>();
        h->Bind(nullptr, 0);
        return h;
    }

    void RenderThreadManager::WaitForLastCommandGPU() {
        for (auto& [t, q] : m_mQueues) q->WaitIdle();
    }

    void RenderThreadManager::WaitForLastCommandGPU(D3D12_COMMAND_LIST_TYPE type) {
        if (auto* q = GetQueue(type)) q->WaitIdle();
    }


    void RenderThreadManager::WaitIdle() {
        m_xCommandsBuilder.WaitIdle();
        m_xRenderThread.WaitIdle();
        WaitForLastCommandGPU();
    }

    void RenderThreadManager::GarbageCollectAll() {
        for (auto& [t, q] : m_mQueues) q->GarbageCollect();
    }

    std::shared_ptr<ExecutionHandle> RenderThreadManager::Submit(const std::shared_ptr<ICommandRecipe>& recipe, std::vector<std::shared_ptr<ExecutionHandle>> dependencies, bool IsNeedFullExecute)
    {
        if (!recipe) {
            auto h = std::make_shared<ExecutionHandle>();
            h->Bind(nullptr, 0);
            return h;
        }
        auto batch = std::make_shared<ClosedBatch>();
        batch->Handle = std::make_shared<ExecutionHandle>();
        batch->Handle->MustFullExecuteWait(IsNeedFullExecute);

        m_xCommandsBuilder.PostTask([recipe, batch, deps = std::move(dependencies), this]() {
            auto* pool = m_mPools[recipe->GetType()].get();
            auto* q = PickCommandQueue(recipe->GetType());
            
            for (auto& d : deps) {
                if (d && d->IsMustFullExecuteWait()) d->FutureWait();
            }

            auto list = pool->BuildFromRecipe(recipe);
            batch->Lists.push_back(std::move(list));

            batch->Recycle = [pool](std::unique_ptr<ICommandList> l) {
                pool->Recycle(std::move(l));
            };

            m_xRenderThread.PostTask(
                {
                    [q, batch, deps]() {
                        for (auto& d : deps) {
                            if (d && d->GetFence()) q->WaitFence(d);
                        }
                        q->Submit(batch);
                    },
                    deps
                }
            );
        });

        
        return batch->Handle;
    }

    std::shared_ptr<ExecutionHandle> RenderThreadManager::SubmitLambda(std::function<void()> func, std::vector<std::shared_ptr<ExecutionHandle>> dependencies, bool IsNeedFullExecute)
    {
        auto handle = std::make_shared<ExecutionHandle>();
        handle->MustFullExecuteWait(IsNeedFullExecute);

        m_xCommandsBuilder.PostTask([func, handle, deps = std::move(dependencies), this]() {
            for (auto& d : deps) {
                if (d) d->WaitForExecute();
            }

            m_xRenderThread.PostTask(
                {
                    [func = std::move(func), handle]() mutable {
                        if (func) func();
                        handle->Bind(nullptr, 0);
                    }
                }
            );
        });

        return handle;
    }

}
