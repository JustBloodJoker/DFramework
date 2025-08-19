#pragma once

#include "../../pch.h"
#include "../ICommandList.h"
#include "ExecutionHandle.h"
#include "WinWindow/Utils/WorkerThread.h"
#include "../BaseCommandQueue.h"
#include "../CommandListTemplate.h"

namespace FD3DW {

	struct ClosedBatch {
		std::vector<std::unique_ptr<ICommandList>> Lists;
		std::shared_ptr<ExecutionHandle> Handle;
		UINT64 FenceValue = 0;
	};

	class AsyncCommandQueue : public BaseCommandQueue {
	public:
		AsyncCommandQueue(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE type);
		virtual ~AsyncCommandQueue();


	public:

		void Submit(std::shared_ptr<ClosedBatch> batch);
		UINT64 GetLoadApprox() const;
		UINT64 SignalFence();
		void WaitFence(UINT64 value, AsyncCommandQueue* fromQueue);
		void WaitIdle();

	private:
		void GarbageCollect();

	private:
		std::atomic<UINT64> m_uLastFenceEvent = 0;
		FDWWIN::WorkerThread m_xWorker;

		std::mutex m_xMutex;
		std::deque<std::shared_ptr<ClosedBatch>> m_dInFlight; 
	};




}