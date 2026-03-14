#include "WorkerThread.h"

namespace FDWWIN {

	void WorkerThread::PostTask(WorkerTask task) {
		{
			std::unique_lock<std::mutex> lock(m_xMutex);
			m_qTasks.push(std::move(task));
		}
		m_xCV.notify_one();
	}

	void WorkerThread::WaitIdle()
	{
		std::unique_lock<std::mutex> lock(m_xMutex);
		m_xIdleCV.wait(lock, [this]() { return m_qTasks.empty(); });
	}

	void WorkerThread::Stop() { 
		{ 
			std::unique_lock<std::mutex> lock(m_xMutex); 
			m_bRunning = false; 
			m_xCV.notify_all(); 
		} 
		
		Thread::Stop();
	}

	WorkerThread::WorkerThread() {}

	WorkerThread::~WorkerThread()
	{
		Stop();
	}

	size_t WorkerThread::GetQueueSize()
	{
		return m_qTasks.size();
	}

	void WorkerThread::ThreadLoop()
	{
		while (true)
		{
			WorkerTask task;
			{
				std::unique_lock<std::mutex> lock(m_xMutex);
				m_xCV.wait(lock, [this]() { return !m_qTasks.empty() || !m_bRunning; });

				if (!m_bRunning && m_qTasks.empty())
					break;

				task = std::move(m_qTasks.front());
			}

			if (task) task();

			{
				std::unique_lock<std::mutex> lock(m_xMutex);
				m_qTasks.pop();
				if (m_qTasks.empty())
					m_xIdleCV.notify_all();
			}
		}

	}
}