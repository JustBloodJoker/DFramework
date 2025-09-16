#pragma once

#include "../pch.h"

namespace FDWWIN {


    class Thread {
    public:
        Thread();
        virtual ~Thread();

        void Start();
        virtual void Stop();

    protected:
        virtual void ThreadLoop() = 0;

    protected:
        std::thread m_xThread;
        std::atomic<bool> m_bRunning;
    };



}
