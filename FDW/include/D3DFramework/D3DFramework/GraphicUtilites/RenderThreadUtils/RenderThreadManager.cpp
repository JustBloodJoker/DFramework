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

    void RenderThreadManager::WaitForCurrentCommandGPU() {
        for (auto& [t, q] : m_mQueues) q->WaitIdle();
    }

    void RenderThreadManager::WaitForCurrentCommandGPU(D3D12_COMMAND_LIST_TYPE type) {
        if (auto* q = GetQueue(type)) q->WaitIdle();
    }


    void RenderThreadManager::WaitIdle() {
        m_xRenderThread.WaitIdle();
    }

    void RenderThreadManager::GarbageCollectAll() {
        for (auto& [t, q] : m_mQueues) q->GarbageCollect();
    }

    std::shared_ptr<ExecutionHandle> RenderThreadManager::Submit(const std::shared_ptr<ICommandRecipe>& recipe, std::vector<std::shared_ptr<ExecutionHandle>> dependencies)
    {
        if (!recipe) {
            auto h = std::make_shared<ExecutionHandle>();
            h->Bind(nullptr, 0);
            return h;
        }
        auto batch = std::make_shared<ClosedBatch>();
        batch->Handle = std::make_shared<ExecutionHandle>();

        m_xRenderThread.PostTask([recipe, batch, this, dependencies]() {
            auto* q = PickCommandQueue(recipe->GetType());
            auto* pool = m_mPools[recipe->GetType()].get();

            auto list = pool->BuildFromRecipe(recipe);
            batch->Lists.push_back(std::move(list));
            batch->Recycle = [pool](std::unique_ptr<ICommandList> l) { 
                pool->Recycle(std::move(l)); 
            };

            for (auto& d : dependencies) if (d && d->GetFence()) q->WaitFence(d);

            q->Submit(batch);      
        });

        
        return batch->Handle;
    }

    std::shared_ptr<ExecutionHandle> RenderThreadManager::SubmitLambda(std::function<void()> func, std::vector<std::shared_ptr<ExecutionHandle>> dependencies)
    {
        auto handle = std::make_shared<ExecutionHandle>();

        m_xRenderThread.PostTask([this,func = std::move(func), deps = std::move(dependencies), handle]() mutable {
            for (auto& d : deps) {
                if (d) d->WaitForExecute();
            }

            if (func) func();

            handle->Bind(nullptr, 0); 
        });

        return handle;
    }

    std::shared_ptr<ExecutionHandle> RenderThreadManager::SubmitChain( const std::vector<std::shared_ptr<ICommandRecipe>>& recipes, std::vector<std::shared_ptr<ExecutionHandle>> dependencies)
    {
        auto handle = std::make_shared<ExecutionHandle>();
        if (recipes.empty()) {
            handle->Bind(nullptr, 0);
            return handle;
        }
        m_xRenderThread.PostTask([handle, recipes, this, dependencies]() {

            AsyncCommandQueue* prevQ = nullptr;
            UINT64 prevFence = 0;

            for (auto& r : recipes) {
                auto* q = PickCommandQueue(r->GetType());
                auto* pool = m_mPools[r->GetType()].get();

                auto list = pool->BuildFromRecipe(r);

                auto batch = std::make_shared<ClosedBatch>();
                batch->Handle = handle;
                batch->Lists.push_back(std::move(list));
                batch->Recycle = [pool](std::unique_ptr<ICommandList> l) { pool->Recycle(std::move(l)); };

                if (prevQ && prevQ != q) {
                    if (prevFence == 0) prevFence = prevQ->ReserveFenceTicket();
                    q->WaitFence(prevFence, prevQ);
                    prevFence = 0;
                }
                else if (!prevQ) {
                    for (auto& d : dependencies) if (d) q->WaitFence(d);
                }

                q->Submit(batch);
                prevQ = q;
            }
        });

        return handle;
    }

}
