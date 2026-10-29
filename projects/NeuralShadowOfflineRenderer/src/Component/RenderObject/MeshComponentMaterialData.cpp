#include "pch.h"

#include <Component/RenderObject/MeshComponentMaterialData.h>

void MeshComponentMaterialData::operator=(const FD3DW::MaterialFrameWork& frameworkData) {
	Diffuse = frameworkData.Diffuse;
	Ambient = frameworkData.Ambient;
	Emissive = frameworkData.Emissive;
	Specular = frameworkData.Specular;
	Roughness = frameworkData.Roughness;
	Metalness = frameworkData.Metalness;
	SpecularPower = frameworkData.SpecularPower;
	HeightScale = frameworkData.HeightScale;
}
