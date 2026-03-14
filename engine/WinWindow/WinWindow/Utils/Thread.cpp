#include "Thread.h"

namespace FDWWIN {




	Thread::Thread() : m_bRunning(false) {}

	Thread::~Thread() {
		Stop();
	}

	void Thread::Start() {
		m_bRunning = true;
		m_xThread = std::thread([this]() { ThreadLoop(); });
	}

	void Thread::Stop() {
		m_bRunning = false;
		if (m_xThread.joinable()) {
			m_xThread.join();
		}
	}

}
