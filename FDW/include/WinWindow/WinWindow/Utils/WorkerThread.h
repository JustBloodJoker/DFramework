#pragma once

#include "../pch.h"
#include "Thread.h"

namespace FDWWIN {

	class WorkerThread : public Thread {
	public:
		WorkerThread();
		virtual ~WorkerThread();

	public:
		size_t GetQueueSize();

	public:
		using WorkerTask = std::function<void()>;
		void PostTask(WorkerTask task);

		void WaitIdle();

		virtual void Stop() override;

	private:
		virtual void ThreadLoop() override;

	protected:
		std::mutex m_xMutex;
		std::condition_variable m_xCV;
		std::condition_variable m_xIdleCV;

		std::queue<WorkerTask> m_qTasks;
	};

}
