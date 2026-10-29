# DFramework

DFramework is a C++ rendering project built around DirectX 12. It includes a realtime renderer, reusable engine components, an offline reference renderer, and Python tools for neural rendering experiments.

## Project Structure

| Directory | Description |
|---|---|
| `engine/D3DFramework` | DirectX 12 rendering framework |
| `engine/WinWindow` | Windowing and input framework |
| `projects/FDW` | Main realtime rendering application |
| `projects/NeuralShadowOfflineRenderer` | Offline ray-traced reference rendering and dataset capture |
| `projects/NeuralShadowTraining` | Python tools for neural models, datasets, and validation |
| `external/imgui` | Third-party UI library |

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++, MSVC v143, and Windows 10 SDK
- CUDA Toolkit 12.9
- CMake with Visual Studio 2022 support
- vcpkg with `assimp:x64-windows`
- A compatible DirectX 12 GPU (DXR and DirectML support are needed for the corresponding rendering modes)

### vcpkg Setup

```powershell
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg.exe install assimp:x64-windows
.\vcpkg.exe integrate install
```

## Building and Running

### Visual Studio

1. Open `FDW.sln` in Visual Studio 2022.
2. Select `Debug | x64` or `Release | x64`.
3. Build the solution.
4. Set `FDW` as the startup project and run it.

Executables:

| Configuration | Output |
|---|---|
| Debug | `x64/Debug/FDW.exe` |
| Release | `x64/Release/FDW.exe` |

### CMake

Configure the project from the repository root:

```powershell
cmake --preset vs2022-x64
```

Build FDW:

```powershell
cmake --build --preset vs2022-x64-debug --target FDW
```

For Release:

```powershell
cmake --build --preset vs2022-x64-release --target FDW
```

The CMake build produces executables under:

```text
out/build/vs2022-x64/projects/FDW/<Configuration>/FDW.exe
```

To build all configured project targets, including test targets:

```powershell
cmake --build --preset vs2022-x64-debug
```

Runtime content is stored in `projects/FDW/Content`. The CMake build automatically copies the required content near the application executable. Visual Studio builds also use vcpkg app-local deployment for the Assimp dependencies.

## Running Tests

### C++ / CTest

After configuring and building the project with CMake, run the registered tests from the repository root.

Debug:

```powershell
ctest --test-dir out/build/vs2022-x64 -C Debug --output-on-failure
```

Release:

```powershell
cmake --build --preset vs2022-x64-release
ctest --test-dir out/build/vs2022-x64 -C Release --output-on-failure
```

Use `--output-on-failure` to display detailed output for failing tests.

### Python Tests

The neural rendering tools use Python and can be installed in an isolated virtual environment.

```powershell
cd projects/NeuralShadowTraining

python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -e .
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
```

The Python package also provides model-inspection and ONNX Runtime parity utilities through the `fdw-neural-shadows` command-line tool.

## Neural Soft Shadows

FDW includes an experimental neural soft-shadow rendering system using ONNX Runtime and DirectML.

The renderer supports its existing hard-shadow path, ray-traced reference rendering, and neural visibility reconstruction. Shadow rendering options are available in the application's renderer settings.

The neural model is stored at:

```text
projects/FDW/Content/ShadowRendering/NeuralModel/neural_shadow_model.onnx
```

Neural inference is initialized when the corresponding rendering mode is selected. The renderer retains a hard-shadow fallback when neural rendering is unavailable.

The neural rendering implementation is experimental and is not presented as a production-ready realtime solution.

## Offline Renderer

`NeuralShadowOfflineRenderer` is a separate project for generating physical ray-traced reference images and capturing data for neural rendering experiments.

Its configuration example is located at:

```text
projects/NeuralShadowOfflineRenderer/Content/CaptureConfigs/capture.example.ini
```

Use a separate output directory for captured data. The offline renderer produces reference visibility independently of the realtime renderer's low-resolution hard-shadow approximation.

## Screenshots

<img width="1253" height="934" alt="img1" src="https://github.com/user-attachments/assets/8c4c1e2e-7c9e-4193-84e5-f2bff4bb457b" />
<img width="1672" height="941" alt="img3" src="https://github.com/user-attachments/assets/1ff10705-b0e5-427b-8d78-fc3706b0f1a3" />
<img width="1672" height="941" alt="img2" src="https://github.com/user-attachments/assets/afbde798-950d-4825-9e52-acaac07d517c" />

