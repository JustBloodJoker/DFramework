#include <System/AudioSystem.h>

std::shared_ptr<FD3DW::ExecutionHandle> AudioSystem::OnStartTick() {
	return GlobalRenderThreadManager::GetInstance()->SubmitLambda([]() {
		//empty call	
	});
}
