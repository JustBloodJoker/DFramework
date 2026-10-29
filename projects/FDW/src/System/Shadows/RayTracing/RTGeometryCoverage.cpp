#include "pch.h"

#include <System/Shadows/RayTracing/RTGeometryCoverage.h>

UINT ToGeometryCoverageBits(NSRTGeometryCoverageFlags flags) {
	return static_cast<UINT>(flags);
}

NSRTGeometryCoverageFlags operator| (NSRTGeometryCoverageFlags lhs, NSRTGeometryCoverageFlags rhs) {
	return static_cast<NSRTGeometryCoverageFlags>(ToGeometryCoverageBits(lhs) | ToGeometryCoverageBits(rhs));
}

bool HasGeometryCoverageFlag(UINT flags, NSRTGeometryCoverageFlags flag) {
	return (flags & ToGeometryCoverageBits(flag)) != 0u;
}

bool IsGroundTruthGeometryEligible(const NSRTGeometryCoverageState& state) {
	return state.ActiveTriangleInstances > 0u &&
		state.ConservativeAlphaOrOpacityInstances == 0u &&
		state.MissingBLASInstances == 0u &&
		state.ProceduralInstances == 0u &&
		state.TLASComplete != 0u;
}
