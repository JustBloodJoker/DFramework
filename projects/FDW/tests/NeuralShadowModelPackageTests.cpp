#include "pch.h"

#include <ShadowData/NeuralShadowData.h>
#include <System/Shadows/Neural/NeuralShadowModelPackage.h>

#ifndef FDW_NEURAL_MODEL_TEST_GRAPH
#define FDW_NEURAL_MODEL_TEST_GRAPH L"Content/ShadowRendering/NeuralModel/neural_shadow_model.onnx"
#endif

int main() {
    using namespace FDW_SD::Model;
    if (BASE_CHANNELS != 9u || DESCRIPTOR_CHANNELS != 18u)
        return 1;
    if (EMITTER_OFFSETS[0].U != -0.6f || EMITTER_OFFSETS[0].V != -0.6f)
        return 2;
    if (EMITTER_OFFSETS[4].U != 0.0f || EMITTER_OFFSETS[4].V != 0.0f)
        return 3;
    if (EMITTER_OFFSETS[8].U != 0.6f || EMITTER_OFFSETS[8].V != 0.6f)
        return 4;
    if (HitChannel(8u) != 8u || DistanceChannel(0u) != 9u || DistanceChannel(8u) != 17u)
        return 5;

    const float low[] = {0.0f, 1.0f, 1.0f, 0.0f};
    if (std::abs(SampleHardVisibility(low, 2u, 2u, 0u, 0u)) > 1e-7f || std::abs(SampleHardVisibility(low, 2u, 2u, 7u, 7u)) > 1e-7f ||
        std::abs(SampleHardVisibility(low, 2u, 2u, 3u, 3u) - 0.46875f) > 1e-6f)
        return 6;

    NeuralShadowModel::Package package;
    std::string error;
    if (!NeuralShadowModel::PackageLoader::LoadAndValidate(FDW_NEURAL_MODEL_TEST_GRAPH, package, error)) {
        std::cerr << error << '\n';
        return 8;
    }
    if (package.Architecture != "SpatialVisibilityModel" || package.RuntimeModelId != "SpatialVisibilityModel" ||
        package.RuntimeRole != "FROZEN_RUNTIME_MODEL" || package.ParameterCount != 59661u || package.ReceptiveField != 33u ||
        package.ReceptiveFieldRadius != 16u || package.InputLayout != "BASE9_DESCRIPTOR18_HARD1" ||
        package.OutputLayout != "PHYSICAL_VISIBILITY")
        return 7;

    if (NeuralShadowModel::PackageLoader::ResolvePhysicalVisibility(-1.0f) != 0.0f ||
        NeuralShadowModel::PackageLoader::ResolvePhysicalVisibility(2.0f) != 1.0f)
        return 9;
    return 0;
}
