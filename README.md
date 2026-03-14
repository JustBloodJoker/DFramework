# DFramework

Realtime rendering project with:
- `projects/FDW` - main engine/application
- `engine/WinWindow` - window/input framework
- `engine/D3DFramework` - rendering framework
- `external/imgui` - third-party UI library

## Prerequisites

- Windows 10/11 x64
- Visual Studio 2022 with:
  - `Desktop development with C++`
  - MSVC v143
  - Windows SDK 10.x
- CUDA Toolkit 12.9
- `vcpkg` with `assimp:x64-windows` installed

### vcpkg quick setup

```powershell
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg.exe install assimp:x64-windows
.\vcpkg.exe integrate install
```

## Build With Visual Studio Solution

Open `FDW.sln` in Visual Studio and build for `x64`.

### Debug

1. Select configuration: `Debug | x64`
2. Build solution (`Ctrl+Shift+B`)
3. Run startup project `FDW` (`F5`)

Output binary:
- `x64/Debug/FDW.exe`

### Release

1. Select configuration: `Release | x64`
2. Build solution
3. Run `FDW`

Output binary:
- `x64/Release/FDW.exe`

## Build With CMake Presets

Project includes ready-to-use preset `vs2022-x64`.

### Configure

```powershell
cmake --preset vs2022-x64
```

### Debug build

```powershell
cmake --build --preset vs2022-x64-debug --target FDW
```

Output binary:
- `out/build/vs2022-x64/projects/FDW/Debug/FDW.exe`

### Release build

```powershell
cmake --build --preset vs2022-x64-release --target FDW
```

Output binary:
- `out/build/vs2022-x64/projects/FDW/Release/FDW.exe`

## Runtime Content

- Source assets are in `projects/FDW/Content`
- For CMake build, `Content` is copied automatically near `FDW.exe` after build

## Notes

- If `cmake` is not in `PATH`, run from `x64 Native Tools Command Prompt for VS 2022`, or use Visual Studio bundled CMake binary.
- If build logs mention missing `pwsh.exe`, MSBuild usually falls back to Windows PowerShell and continues.
