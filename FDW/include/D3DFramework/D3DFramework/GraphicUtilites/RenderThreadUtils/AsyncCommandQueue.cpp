#include "AsyncCommandQueue.h"
#include "RenderThreadManager.h"

namespace FD3DW {
	
	AsyncCommandQueue::AsyncCommandQueue(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE type) : BaseCommandQueue(dev, type) {
		m_xWorker.Start();
	}

	AsyncCommandQueue::~AsyncCommandQueue()
	{
		m_xWorker.Stop();
	}

    void AsyncCommandQueue::Submit(std::shared_ptr<ClosedBatch> batch) {
		m_xWorker.PostTask([this, batch = batch]() {
            if (!batch->Lists.empty()) {
				std::vector<ID3D12CommandList*> lists;
				for (const auto& list : batch->Lists) {
					lists.push_back(list->GetPtrDefaultCommandList());
				}

                m_pCommandQueue->ExecuteCommandLists((UINT)lists.size(), lists.data());
            }

			batch->FenceValue = SignalFence();
            if (batch->Handle) batch->Handle->Bind(m_pFence.Get(), batch->FenceValue);
			
			{
				std::scoped_lock lock(m_xMutex);
				m_dInFlight.push_back(batch);
			}

			//GarbageCollect();
        });
    }

	UINT64 AsyncCommandQueue::GetLoadApprox() const
	{
		return m_uLastFenceEvent - m_pFence->GetCompletedValue();
	}

	void AsyncCommandQueue::WaitFence(UINT64 value, AsyncCommandQueue* fromQueue)
	{
		HRESULT_ASSERT(m_pCommandQueue->Wait(fromQueue->GetFence(), value), "WaitFence error");
	}

	void AsyncCommandQueue::WaitIdle()
	{
		auto exHandle = std::make_shared<ExecutionHandle>();
		exHandle->Bind(m_pFence.Get(), m_uLastFenceEvent);
		exHandle->WaitForExecute();
	}

	UINT64 AsyncCommandQueue::SignalFence()
	{
		UINT64 value = ++m_uLastFenceEvent;
		HRESULT_ASSERT(m_pCommandQueue->Signal(m_pFence.Get(), value), "Signal error");
		return m_uLastFenceEvent;
	}

	void AsyncCommandQueue::GarbageCollect() {
		const UINT64 completed = m_pFence->GetCompletedValue();
		std::scoped_lock lock(m_xMutex);

		while (!m_dInFlight.empty()) {
			auto& b = m_dInFlight.front();
			if (b->FenceValue <= completed) {
				m_dInFlight.pop_front();
			}
			else {
				break;
			}
		}
	}

}
