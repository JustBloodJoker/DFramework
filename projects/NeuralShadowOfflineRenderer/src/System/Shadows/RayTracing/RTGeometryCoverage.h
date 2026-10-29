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
	ProceduralGeometry = 1u << 6u,
	AlphaTestedTriangles = 1u << 7u
};

UINT ToGeometryCoverageBits(NSRTGeometryCoverageFlags flags);

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

enum class NSRTGeometryAlphaMode : UINT {
	Opaque = 0u,
	Mask = 1u,
	Blend = 2u,
	Unknown = 3u
};

enum class NSRTGeometrySupportClass : UINT {
	OpaqueTriangles = 0u,
	AlphaTestedTriangles = 1u,
	UnsupportedAlphaOrOpacity = 2u
};

struct NSRTGeometryMaterialFacts {
	NSRTGeometryAlphaMode AlphaMode = NSRTGeometryAlphaMode::Opaque;
	UINT HasBaseColorTexture = 0u;
	UINT HasOpacityTexture = 0u;
	UINT HasTextureCoordinates = 0u;
	UINT HasCompatibleSharedBuffers = 0u;
	
	float AlphaCutoff = 0.5f;
	float OpacityFactor = 1.0f;
};

bool IsFiniteUnitValue(float value);

NSRTGeometrySupportClass ClassifyGeometryMaterial(const NSRTGeometryMaterialFacts& facts);

enum class NSRTGeometryInstanceFlags : UINT {
	None = 0u,
	AlphaTested = 1u << 0u,
	DoubleSided = 1u << 1u
};

struct NSRTGeometryInstance {
	UINT IndexOffset = 0u;
	UINT VertexOffset = 0u;
	UINT VertexStride = 0u;
	UINT AlphaTextureIndex = 0u;
	
	float AlphaCutoff = 0.5f;
	float OpacityFactor = 1.0f;
	
	UINT Flags = 0u;
	UINT Padding = 0u;
};

struct NSRTGeometryAccounting {
	UINT TotalInstances = 0u;
	UINT OpaqueInstances = 0u;
	UINT AlphaTestedInstances = 0u;
	UINT UnsupportedAlphaOrOpacityInstances = 0u;
	UINT DoubleSidedInstances = 0u;
	UINT SkinnedInstances = 0u;
	UINT MissingBLASInstances = 0u;
	UINT TriangleCount = 0u;
	UINT SharedAlphaBuffersCompatible = 0u;
};

bool IsGroundTruthGeometryEligible(const NSRTGeometryCoverageState& state);
