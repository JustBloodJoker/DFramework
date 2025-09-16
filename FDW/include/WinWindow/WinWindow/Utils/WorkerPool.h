#pragma once

#include "../pch.h"
#include "WorkerThread.h"

namespace FDWWIN {



class WorkerPool {
public:
	WorkerPool() = default;
	virtual ~WorkerPool() = default;

public:

	void Init(int maxWorkersCount);
	void PostTask(WorkerThread::WorkerTask task);
	void WaitIdle();

	std::vector<WorkerThread*> GetWorkers() const;

protected:
	std::unique_ptr<WorkerThread>& GetWorker();

protected:
	std::vector<std::unique_ptr<WorkerThread>> m_vWorkers;
	int m_iMaxWorkersCount = 1;

};



}
