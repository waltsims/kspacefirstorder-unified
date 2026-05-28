# Issue 17 Plan: Windows CUDA DLL Packaging

## Goal

Fix the Windows CUDA release artifact so `kspaceFirstOrder-CUDA.exe` ships with the runtime DLLs needed on a clean Windows machine, then validate locally before relying on GitHub Actions.

Primary issue: https://github.com/waltsims/kspacefirstorder-unified/issues/17

## Plain-English Summary

Walter asked us to fix the Windows CUDA binary package because the current v1.4.x CUDA release ships only `kspaceFirstOrder-CUDA.exe`, without the DLL files that executable needs on a clean Windows machine.

In simple terms, we are going to:

1. Build the Windows CUDA executable locally on this laptop.
2. Check which DLL files it needs.
3. Copy those DLL files into the same folder as `kspaceFirstOrder-CUDA.exe`.
4. Test from a clean folder so we know the executable works because the DLLs are bundled, not because this laptop happens to have CUDA installed globally.
5. Update GitHub Actions so future Windows CUDA artifacts include those DLLs automatically.
6. Download and test the GitHub Actions artifact locally.
7. Only then update `k-wave-python` to use the newer Windows binaries.

This matches Walter's request because issue #17 says the v1.4.x Windows CUDA release is missing runtime DLLs, and Walter specifically asked Farid to validate Windows OpenMP on real hardware and fix Windows CUDA packaging using the same DLL-bundling pattern as Windows OpenMP, plus CUDA runtime DLLs from `$env:CUDA_PATH\bin`.

The conservative sequencing is intentional: do not flip `k-wave-python` from v1.3.0 to v1.4.x/v1.4.2 until the fixed Windows CUDA artifact exists and has passed local testing.

## Current Local Baseline

- `kspacefirstorder-unified`: branch `fix/issue-17-windows-cuda-dll-packaging`, based on `main` at `7e2cc66`.
- `k-wave-python`: `master` at `66c256d`, clean; do not change until the unified binary artifact is fixed and validated.
- Machine: Windows 11 Home, Intel i7-13700H, NVIDIA RTX 4060 Laptop GPU, driver `560.76`, CUDA driver support `12.6`, CUDA Toolkit `12.6`.
- Local CUDA DLL names observed: `cudart64_12.dll`, `cufft64_11.dll`, `cufftw64_11.dll`.
- Workspace-local tools installed:
  - Ninja: `C:\Users\offic\Documents\projects\_tools\ninja\ninja.exe`, version `1.13.2`.
  - uv: `C:\Users\offic\Documents\projects\_tools\uv\uv.exe`, version `0.11.16`.
  - vcpkg: `C:\Users\offic\Documents\projects\_tools\vcpkg\vcpkg.exe`, version `2026-04-08-e0612b42`.
- Python work must happen inside a virtual environment. Prefer uv-managed venvs for local Python testing.

## Local-First Execution Plan

1. Inspect current CI packaging.
   - Use the existing `windows-openmp` DLL collection step as the template.
   - Confirm where the `windows-cuda` CMake build places `vcpkg_installed/x64-windows/bin`.
   - Confirm CUDA binary runtime dependencies with `dumpbin /dependents` or equivalent from the VS developer environment.

2. Build Windows CUDA locally.
   - Use Visual Studio 2022 developer environment.
   - Use local vcpkg root at `C:\Users\offic\Documents\projects\_tools\vcpkg`.
   - Configure `repos/kspaceFirstOrder-cuda-windows` into `build/cuda-windows`.
   - Build Release with MSBuild/CMake.

3. Prototype local CUDA DLL bundling.
   - Copy vcpkg runtime DLLs from the CUDA build's `vcpkg_installed/x64-windows/bin`.
   - Copy MSVC CRT redistributable DLLs only from `VCToolsRedistDir` using the `Microsoft.*.CRT` folder pattern.
   - Copy CUDA runtime DLLs from `$env:CUDA_PATH\bin`, at minimum `cudart64_*.dll` and `cufft64_*.dll`.
   - Keep OpenMP-specific redistributables out of the CUDA bundle unless dependency inspection proves otherwise.

4. Validate the local CUDA artifact.
   - Stage `kspaceFirstOrder-CUDA.exe` and bundled DLLs into a clean temp directory.
   - Run smoke tests with a sanitized `PATH` so CUDA/vcpkg/VS install directories do not hide missing bundled DLLs.
   - Run at least `--help`; if feasible, run a small GPU-backed k-wave-python simulation from a venv.

5. Validate Windows OpenMP on this machine.
   - Build or use the existing Windows OpenMP workflow-equivalent path.
   - Smoke test `kspaceFirstOrder-OMP.exe` locally on real Windows hardware.
   - Record whether v1.4.x/v1.4.2 OpenMP is viable for the later `k-wave-python` pin flip.

## Code Changes After Local Proof

1. Update `.github/workflows/ci-multi-platform.yml`.
   - Add a `Collect runtime DLLs (CUDA Windows)` step after the Windows CUDA build.
   - Copy vcpkg DLLs, MSVC CRT DLLs, and CUDA DLLs.
   - Update the Windows CUDA `upload-artifact` path to include `build/cuda-windows/Release/*.dll`.
   - Add a Windows CUDA smoke test that executes the staged `.exe` after DLL collection.

2. Update `.github/workflows/release-on-tag.yml`.
   - Stage DLLs from the Windows CUDA artifact as well as the Windows OpenMP artifact.
   - Avoid assuming the OpenMP artifact is sufficient for CUDA-specific DLLs.

3. Run local workflow-equivalent checks again.
   - Re-run CUDA build, collection, and smoke test.
   - Re-run any changed PowerShell snippets locally before pushing.

## GitHub Actions Phase

1. Push `fix/issue-17-windows-cuda-dll-packaging`.
2. Open a PR against `main`.
3. Run the multi-platform CI workflow.
4. Download the Windows CUDA artifact from the PR run.
5. Validate the downloaded artifact locally with sanitized `PATH`.
6. If clean, coordinate release tag/dry run for the unified binary release.

## k-wave-python Phase

Only start this after the unified Windows CUDA release artifact includes the required DLLs and passes local validation.

1. Create a separate branch in `k-wave-python`.
2. Use a Python virtual environment for all package/test work.
3. Update Windows binary pins from `v1.3.0` to the validated unified release version.
4. Update `WINDOWS_DLLS` for CUDA 12-era names such as `cudart64_12.dll` and `cufft64_11.dll`, or refactor to per-backend DLL lists if OpenMP and CUDA dependency sets diverge.
5. Run focused Windows CPU/GPU execution tests locally before pushing.

## Risks And Checks

- CUDA Toolkit on this machine is `12.6`, while CI currently targets CUDA `13.0.0`; local DLL names may differ from CI artifact DLL names.
- This machine's normal `PATH` includes CUDA tooling, so clean validation must sanitize `PATH`.
- GitHub-hosted Windows runners may have different MSVC redist layout; CI collection should fail loudly if expected directories are missing.
- `k-wave-python` should not be pinned forward until both Windows OpenMP and Windows CUDA artifacts are validated.
