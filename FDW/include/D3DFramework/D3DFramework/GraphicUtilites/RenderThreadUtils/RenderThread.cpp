#include "RenderThread.h"

namespace FD3DW {
    RenderThread::~RenderThread() { Stop(); }

    void RenderThread::PostTask(RenderTask task)
    {
        {
            std::unique_lock<std::mutex> lock(m_xMutex);
            m_qTasks.push(std::move(task));
        }
        m_xCV.notify_one();
    }

    void RenderThread::WaitIdle() {
        std::unique_lock<std::mutex> lock(m_xMutex);
        m_xIdleCV.wait(lock, [this]() { return m_qTasks.empty(); });
    }

    void RenderThread::Stop()
    {   
        {
            std::unique_lock<std::mutex> lock(m_xMutex);
            m_bRunning = false;
            m_xCV.notify_all();
        }

        Thread::Stop();
    }

    int RenderThread::GetTasksCount()
    {
        return int( m_qTasks.size() );
    }

    void RenderThread::ThreadLoop()
    {
        while (m_bRunning) {
            RenderTask task;

            {
                std::unique_lock<std::mutex> lock(m_xMutex);

                m_xCV.wait(lock, [this]() {
                    return !m_qTasks.empty() || !m_bRunning;
                });

                if (!m_bRunning && m_qTasks.empty())
                    break;

                task = std::move(m_qTasks.front());
                m_qTasks.pop();
            }

            bool ready = true;
            for (auto& d : task.Deps) {
                if (d && !d->IsFutureDone()) {
                    ready = false;
                    break;
                }
            }

            if (!ready) {
                std::unique_lock<std::mutex> lock(m_xMutex);
                m_qTasks.push(std::move(task));
                continue;
            }

            if (task.Task) task.Task();

            {
                std::unique_lock<std::mutex> lock(m_xMutex);
                if (m_qTasks.empty())
                    m_xIdleCV.notify_all();
            }
        }
    }

}
