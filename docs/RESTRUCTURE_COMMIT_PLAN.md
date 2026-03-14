# Project Restructure Commit Plan

This plan keeps history readable and reduces rollback risk.

## Commit 1: Physical move + VS solution relink

Scope:
- Move project layout to `projects/`, `engine/`, `external/`
- Update `FDW.sln` project paths
- Update `projects/FDW/FDW.vcxproj` and `.filters` include/source paths

Suggested checks:
- Open `FDW.sln` in Visual Studio
- Build `Debug|x64`

## Commit 2: CMake bootstrap

Scope:
- Add root and module `CMakeLists.txt`
- Add `CMakePresets.json`
- Ensure `FDW` target links `D3DFramework`, `WinWindow`, `imgui`
- CUDA configuration for `D3DFramework`

Suggested checks:
- `cmake --preset vs2022-x64`
- `cmake --build --preset vs2022-x64-debug --target FDW`

## Commit 3: Runtime packaging polish

Scope:
- Copy `projects/FDW/Content` next to built executable in CMake
- Add `install()` rules for `FDW` and `Content`
- Ignore generated CMake output in `.gitignore` (`out/`)
- Add solution folders for better VS navigation

Suggested checks:
- Run `FDW.exe` from build output and verify asset loading
- Validate Visual Studio solution tree grouping

## Suggested command sequence

```powershell
# Review staged plan first
git status

# Commit 1
git add -A FDW.sln projects/FDW engine/WinWindow engine/D3DFramework external/imgui
git commit -m "Restructure tree into projects/engine/external and relink VS projects"

# Commit 2
git add CMakeLists.txt CMakePresets.json projects/FDW/CMakeLists.txt engine/WinWindow/CMakeLists.txt engine/D3DFramework/CMakeLists.txt external/imgui/CMakeLists.txt
git commit -m "Add CMake build system for FDW, WinWindow, D3DFramework, and imgui"

# Commit 3
git add .gitignore docs/RESTRUCTURE_COMMIT_PLAN.md FDW.sln projects/FDW/CMakeLists.txt
git commit -m "Polish runtime packaging, install rules, and solution organization"
```
