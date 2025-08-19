#include "WorkerThread.h"

namespace FDWWIN {

	WorkerThread::WorkerThread() : m_bRunning(false) {}

	WorkerThread::~WorkerThread() 
	{
		Stop();
	}
	void WorkerThread::Start() {
		m_bRunning = true;
		m_xThread = std::thread([this]() { ThreadLoop(); });
	}

	void WorkerThread::Stop() {
		{
			std::unique_lock<std::mutex> lock(m_xMutex);
			m_bRunning = false;
			m_xCV.notify_all();
		}
		if ( m_xThread.joinable() ) m_xThread.join();
	}

	void WorkerThread::PostTask(WorkerTask task) {
		{
			std::unique_lock<std::mutex> lock(m_xMutex);
			m_qTasks.push(std::move(task));
		}
		m_xCV.notify_one();
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
				m_qTasks.pop();
			}

			if (task) task();
		}
	}
}