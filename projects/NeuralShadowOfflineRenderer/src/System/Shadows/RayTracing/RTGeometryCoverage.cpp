#include "pch.h"

#include <System/Shadows/RayTracing/RTGeometryCoverage.h>

UINT ToGeometryCoverageBits(NSRTGeometryCoverageFlags flags) {
	return static_cast<UINT>(flags);
}

bool IsFiniteUnitValue(float value) {
	return value == value && value >= 0.0f && value <= 1.0f;
}

NSRTGeometrySupportClass ClassifyGeometryMaterial(const NSRTGeometryMaterialFacts& facts) {
	if (facts.AlphaMode == NSRTGeometryAlphaMode::Opaque &&
		facts.HasOpacityTexture == 0u && facts.OpacityFactor >= 1.0f) {

		return NSRTGeometrySupportClass::OpaqueTriangles;
	}

	if (facts.AlphaMode == NSRTGeometryAlphaMode::Mask &&
		facts.HasBaseColorTexture != 0u && facts.HasTextureCoordinates != 0u &&
		facts.HasCompatibleSharedBuffers != 0u && IsFiniteUnitValue(facts.AlphaCutoff) &&
		IsFiniteUnitValue(facts.OpacityFactor) && facts.OpacityFactor > 0.0f) {

		return NSRTGeometrySupportClass::AlphaTestedTriangles;
	}

	return NSRTGeometrySupportClass::UnsupportedAlphaOrOpacity;
}

bool IsGroundTruthGeometryEligible(const NSRTGeometryCoverageState& state) {
	return state.ActiveTriangleInstances > 0u && state.ConservativeAlphaOrOpacityInstances == 0u && state.MissingBLASInstances == 0u && state.ProceduralInstances == 0u && state.TLASComplete != 0u;
}
