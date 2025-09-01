#pragma once

#include "../pch.h"

namespace FDWWIN {

	class WorkerThread {
	public:
		WorkerThread();
		virtual ~WorkerThread();

	public:
		void Start();
		void Stop();
		
	public:
		using WorkerTask = std::function<void()>;
		void PostTask(WorkerTask task);

		void WaitIdle();

	private:
		void ThreadLoop();

	protected:
		std::thread m_xThread;
		std::mutex m_xMutex;
		std::condition_variable m_xCV;
		std::condition_variable m_xIdleCV;

		std::queue<WorkerTask> m_qTasks;

		std::atomic<bool> m_bRunning;
	};

}
