#include "AsyncCommandQueue.h"
#include "RenderThreadManager.h"

namespace FD3DW {
	
	

	AsyncCommandQueue::AsyncCommandQueue(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE type) : BaseCommandQueue(dev, type) {}

	void AsyncCommandQueue::Submit(std::shared_ptr<ClosedBatch> batch)
    {
        const UINT64 ticket = ReserveFenceTicket();
        batch->FenceValue = ticket;

        if (batch->Handle) {
            batch->Handle->Bind(m_pFence.Get(), ticket);
        }

        if (!batch->Lists.empty()) {
            std::vector<ID3D12CommandList*> lists;
            lists.reserve(batch->Lists.size());
            for (const auto& l : batch->Lists) {
                lists.push_back(l->GetPtrDefaultCommandList());
            }
            m_pCommandQueue->ExecuteCommandLists((UINT)lists.size(), lists.data());
        }

        HRESULT_ASSERT(m_pCommandQueue->Signal(m_pFence.Get(), ticket), "Signal error");
        m_uLastFenceEvent.store(ticket, std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock(m_xMutex);
            m_dInFlight.push_back(std::move(batch));
        }
    }

    UINT64 AsyncCommandQueue::GetLoadApprox() const
    {
        return m_uLastFenceEvent - m_pFence->GetCompletedValue();
    }

    UINT64 AsyncCommandQueue::ReserveFenceTicket()
    {
        return m_uNextFenceTicket.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    void AsyncCommandQueue::WaitFence(std::shared_ptr<ExecutionHandle> dependency)
    {
        if (!dependency) return;
        dependency->FutureWait();
        WaitFence(dependency->GetFenceValue(), dependency->GetFence());
    }

    void AsyncCommandQueue::WaitFence(UINT64 value, AsyncCommandQueue* fromQueue)
    {
        WaitFence(value, fromQueue->GetFence());
    }


    void AsyncCommandQueue::WaitIdle() {
        const UINT64 ticket = ReserveFenceTicket();
        HRESULT_ASSERT(m_pCommandQueue->Signal(m_pFence.Get(), ticket), "Signal error");
        m_uLastFenceEvent.store(ticket, std::memory_order_release);

        ExecutionHandle h;
        h.Bind(m_pFence.Get(), ticket);
        h.WaitForExecute();
    }

    std::shared_ptr<ExecutionHandle> AsyncCommandQueue::CreateReservedExecutionHandle()
    {
        auto h = std::make_shared<ExecutionHandle>();
        const UINT64 ticket = ReserveFenceTicket();
        h->Bind(m_pFence.Get(), ticket);
        return h;
    }

    void AsyncCommandQueue::GarbageCollect()
    {
        const UINT64 completed = m_pFence->GetCompletedValue();

        std::lock_guard<std::mutex> lock(m_xMutex);
        while (!m_dInFlight.empty()) {
            auto& b = m_dInFlight.front();
            if (b->FenceValue <= completed) {
                if (b->Recycle) {
                    for (auto& list : b->Lists) {
                        b->Recycle(std::move(list));
                    }
                }
                m_dInFlight.pop_front();
            }
            else {
                break;
            }
        }
    }

    void AsyncCommandQueue::WaitFence(UINT64 value, ID3D12Fence* fence)
    {
        HRESULT_ASSERT(m_pCommandQueue->Wait(fence, value), "WaitFence error");
    }

}
