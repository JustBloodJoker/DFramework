#include "pch.h"

#include <System/Shadows/ShadowRouter.h>

ShadowFrameContext ShadowRouter::Resolve(std::uint64_t frameIndex,ShadowMode requested,ShadowBackendAvailability availability,std::uint32_t lightCount) const {
	ShadowFrameContext context{};
	context.FrameIndex = frameIndex;
	context.RequestedMode = requested;
	context.Routing = ShadowRouting::Resolve( requested, availability.UnlitScene, availability.RayTracingSupported,
											  availability.HardAtlas, availability.RayTraced, availability.CompactBaseline, availability.Neural);
	context.Producers = ShadowRouting::ResolveProducers(context.Routing);
	context.LightCount = lightCount;
	return context;
}
