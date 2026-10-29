#pragma once

#include <Windows.h>

enum class NSRTGeometryCoverageFlags : UINT {
	None = 0u,
	OpaqueTriangles = 1u << 0u,
	TwoSidedThinTriangles = 1u << 1u,
	RigidTransforms = 1u << 2u,
	SkinnedVertexUpdates = 1u << 3u,
	ConservativeAlphaOrOpacityMask = 1u << 4u,
	MissingAccelerationStructure = 1u << 5u,
	ProceduralGeometry = 1u << 6u
};

UINT ToGeometryCoverageBits(NSRTGeometryCoverageFlags flags);

NSRTGeometryCoverageFlags operator| (NSRTGeometryCoverageFlags lhs, NSRTGeometryCoverageFlags rhs);

bool HasGeometryCoverageFlag(UINT flags, NSRTGeometryCoverageFlags flag);

struct NSRTGeometryCoverageState {
	UINT Flags = ToGeometryCoverageBits(NSRTGeometryCoverageFlags::None);
	UINT ActiveTriangleInstances = 0u;
	UINT RigidInstances = 0u;
	UINT SkinnedInstances = 0u;
	UINT ConservativeAlphaOrOpacityInstances = 0u;
	UINT MissingBLASInstances = 0u;
	UINT ProceduralInstances = 0u;
	UINT TLASComplete = 0u;
	UINT GroundTruthGeometryEligible = 0u;
};

bool IsGroundTruthGeometryEligible(const NSRTGeometryCoverageState& state);
