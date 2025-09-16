#pragma once

#include "../../pch.h"
#include "WinWindow/Utils/Thread.h"
#include "ExecutionHandle.h"

namespace FD3DW {


    class RenderThread : public FDWWIN::Thread {
    public:

        struct RenderTask {
            std::function<void()> Task;
            std::vector<std::shared_ptr<ExecutionHandle>> Deps;
        };

        RenderThread() = default;
        virtual ~RenderThread() override;

        void PostTask(RenderTask task);

        void WaitIdle();

        virtual void Stop() override;

        int GetTasksCount();
    protected:
        void ThreadLoop() override;

    private:
        std::mutex m_xMutex;
        std::condition_variable m_xCV;
        std::condition_variable m_xIdleCV;
        std::queue<RenderTask> m_qTasks;
    };

}
