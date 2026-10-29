#include "pch.h"

#include <CaptureApplication.h>

#include <Capture/NeuralShadowCaptureController.h>
#include <Renderer/OfflineRenderer.h>

int CaptureApplication::Run(int argc, char** argv) const {
    std::filesystem::path configPath;
    std::filesystem::path outputRoot;

    bool resume = false;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
    
        if (argument == "--config" && index + 1 < argc) configPath = argv[++index];
        else if (argument == "--output" && index + 1 < argc) outputRoot = argv[++index];
        else if (argument == "--resume") resume = true;
        else return 2;
    }
    
    if (configPath.empty() || outputRoot.empty()) {
        std::cerr << "Usage: NeuralShadowOfflineRenderer.exe --config <capture.ini> "
            "--output <dataset-root> [--resume]\n";
        return 2;
    }

    FDW_SD::Capture::NSCaptureConfig config{};
    const auto load = FDW_SD::Capture::CaptureConfiguration::Load(configPath, config);
    if (!load.Succeeded()) {
        std::cerr << "Failed to load neural-shadow capture config: " << load.Message << '\n';
        return 2;
    }

    NeuralShadowCaptureRunOptions options{};
    options.Config = std::move(config);
    options.DatasetRoot = std::filesystem::absolute(outputRoot);
    options.Resume = resume;
    
    auto controller = std::make_unique<NeuralShadowCaptureController>(std::move(options));
    auto renderer = std::make_unique<OfflineRenderer>(*controller);
    
    renderer->__START();
    
    const auto exitCode = controller->GetExitCode();
    
    renderer.reset();
    controller.reset();
    return exitCode;
}
