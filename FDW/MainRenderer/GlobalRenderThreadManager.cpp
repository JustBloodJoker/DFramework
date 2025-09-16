#include <MainRenderer/GlobalRenderThreadManager.h>

GlobalRenderThreadManager::GlobalRenderThreadManager() {
	m_iMaxThreadsCount = GLOBAL_RENDER_THREAD_MANAGER_WORKER_POOL_MAX_COUNT;
}
