#pragma once

#include "../pch.h"

namespace FDWWIN {
    template<class T>
    class CreativeSingleton {

    protected:
        inline static T* m_pSingletonInstance = nullptr;

        static void NullifyInstance() {
            m_pSingletonInstance = nullptr;
        }

        static bool IsInstance() {
            return m_pSingletonInstance != nullptr;
        }

    public:

        static T* GetInstance() {
            if (!m_pSingletonInstance) m_pSingletonInstance = new T();
            return m_pSingletonInstance;
        }

        static void FreeInstance() {
            delete m_pSingletonInstance;
            NullifyInstance();
        }
    };
}