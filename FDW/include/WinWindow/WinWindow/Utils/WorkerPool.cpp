#include "WorkerPool.h"

namespace FDWWIN {

	void WorkerPool::Init(int maxWorkersCount) {
		m_iMaxWorkersCount = maxWorkersCount;
	}

	void WorkerPool::PostTask(WorkerThread::WorkerTask task) {
		auto& availWorker = GetWorker();
		availWorker->PostTask(task);
	}

	void WorkerPool::WaitIdle() {
		for (auto& worker : m_vWorkers) {
			worker->WaitIdle();
		}
	}


    std::vector<WorkerThread*> WorkerPool::GetWorkers() const {
        std::vector<WorkerThread*> ret;

        for (const auto& worker : m_vWorkers) {
            ret.push_back(worker.get());
        }

        return ret;

    }

    std::unique_ptr<WorkerThread>& WorkerPool::GetWorker()
    {
        if (m_vWorkers.empty()) {
            auto worker = std::make_unique<WorkerThread>();
            worker->Start();
            m_vWorkers.push_back(std::move(worker));
            return m_vWorkers.back();
        }

        auto itBest = m_vWorkers.begin();
        int minQueueSize = int( (*itBest)->GetQueueSize() );

        for (auto it = m_vWorkers.begin(); it != m_vWorkers.end(); ++it) {
            int qSize = int( (*it)->GetQueueSize() );
            if (qSize < minQueueSize) {
                minQueueSize = qSize;
                itBest = it;
            }
        }

        if (minQueueSize > 0 && (int)m_vWorkers.size() < m_iMaxWorkersCount) {
            auto worker = std::make_unique<WorkerThread>();
            worker->Start();
            m_vWorkers.push_back(std::move(worker));
            return m_vWorkers.back();
        }

        return *itBest;
    }





}