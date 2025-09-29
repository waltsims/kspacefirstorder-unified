# Start-Over Plan: Submodules + CI per Subproject

This plan resets our approach with small, verifiable steps. We will:
- Track the five existing projects as git submodules (sub-repos) instead of copying sources in-tree.
- Add a dedicated GitHub Actions workflow that builds each subproject independently across OSes.
- Establish a stable baseline before any merging/refactoring.

Once CI is reliably green for all five, we will consolidate code (shared core, fewer backends, simpler CMake) in incremental PRs.

## Principles
- Keep changes small, reversible, and independently verifiable.
- Don’t break existing CI; add a new workflow for the start-over track.
- Pin submodule SHAs for reproducibility; update intentionally via PR.
- Prefer CMake presets and vcpkg where possible, but minimize dependencies in early steps to get green builds first.

## Target Outcome (Phase 1)
- A new workflow (`ci-start-over.yml`) that builds all five subprojects as-is from submodules on Linux, macOS, and Windows (OpenMP-only to start).
- Each subproject builds from its own CMake entry (or wrapper) with artifacts uploaded.
- No refactors yet; only wiring and automation.

## Repository Layout
We will house submodules under `repos/` to keep them clearly separated from our current source tree.

```
repos/
  kspaceFirstOrder-openmp-linux/     # submodule (Linux CPU/OpenMP)
  kspaceFirstOrder-openmp-windows/   # submodule (Windows CPU/OpenMP)
  kspaceFirstOrder-openmp-darwin/    # submodule (macOS CPU/OpenMP)
  kspaceFirstOrder-cuda-linux/       # submodule (Linux CUDA)
  kspaceFirstOrder-cuda-windows/     # submodule (Windows CUDA)
```

Note: Confirm the exact repository names/URLs. These reflect the five subprojects you specified: OpenMP on linux/windows/darwin and CUDA on windows/linux.

## Step 0 — Inventory and Freeze
Deliverables:
- List the five repos (name, URL, default branch, license):
  - kspaceFirstOrder-openmp-linux — URL: [TBD]
  - kspaceFirstOrder-openmp-windows — URL: [TBD]
  - kspaceFirstOrder-openmp-darwin — URL: [TBD]
  - kspaceFirstOrder-cuda-linux — URL: [TBD]
  - kspaceFirstOrder-cuda-windows — URL: [TBD]
- Record the baseline commit SHA to pin for each submodule.
- For each project: build system (CMake/Make/MSBuild/MATLAB), primary targets, minimal build flags, required dependencies (FFTW/HDF5/CUDA/BLAS, etc.).
- Identify availability of `--version`/`--help` for smoke tests; if absent, note a minimal command to run without input files.

Acceptance:
- A short doc (e.g., `plans/06a-inventory.md`) with the above info and copy-pasteable `git submodule add` commands.

What could go wrong (and mitigations):
- Ambiguous repo names or moved/archived repos → Confirm URLs/owners with maintainers; capture in inventory before adding.
- Non-CMake build systems (e.g., Make/MSBuild/MATLAB) → Note per-repo build commands now; plan wrappers in Step 2.
- License mismatches (LGPL/GPL/proprietary assets) → Record license in inventory; gate merging on compatibility review.
- Branch naming differences (main/master/develop) → Record default branch explicitly; pin to SHA in `.gitmodules`.
- Large repos/test data → Flag for LFS or separate “test-assets” submodule; avoid cloning giant data in CI.
- Windows path length issues → Enforce `core.longpaths=true` on developer machines/CI where needed.

## Step 1 — Add Submodules (no code changes)
Actions:
- Add each upstream as a submodule pinned to the baseline SHA.
  - Example: `git submodule add --branch main <URL> repos/<name>` then `git -C repos/<name> checkout <SHA>`
- Enable shallow clones to keep CI fast: `git config -f .gitmodules submodule.repos/<name>.shallow true`
- Commit `.gitmodules` and submodule pointers.
- Helper: use `scripts/submodules/add_submodules.sh` — fill in URLs/SHAs and run to add all five consistently.

Acceptance:
- `git submodule status` shows all five at the intended SHAs.
- Fresh clone with `--recurse-submodules` completes cleanly.

What could go wrong (and mitigations):
- Submodule path conflicts with existing tree → Use `repos/<name>` namespace; verify no collisions.
- Nested submodules in upstream repos → Use `actions/checkout@v4` with `submodules: recursive`; if heavy, set top-level only and document.
- Private repos or rate limits → Ensure access tokens for CI; prefer public mirrors when possible.
- Shallow submodules missing required history → If tags/describe needed, disable shallow for that repo.
- Line-ending differences (CRLF vs LF) → Respect upstream `.gitattributes`; avoid normalizing in submodules.

## Step 2 — Minimal CI per Subproject (new workflow)
Actions:
- Add `.github/workflows/ci-start-over.yml` that:
  - Checks out with submodules: `actions/checkout@v4` with `submodules: true`.
  - Builds each subproject independently using a matrix over `{project, os}` with explicit names:
    - Stage A: `kspaceFirstOrder-openmp-linux`
    - Stage B: add `kspaceFirstOrder-openmp-windows`, `kspaceFirstOrder-openmp-darwin`
    - Stage C: add `kspaceFirstOrder-cuda-linux` (compile-only), then `kspaceFirstOrder-cuda-windows` (compile-only)
  - For C++ projects, prefer CMake; if upstream lacks CMake, build using its native system (Make/MSBuild) or add a non-invasive wrapper under `wrappers/<project>/` in this repo.
  - Upload produced binaries/libs per project as artifacts.
- Start with Linux only; expand to macOS/Windows once Linux is green.

Example matrix skeleton (to implement in the workflow):

```yaml
name: CI (start-over)

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-subprojects:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: ubuntu-latest
            project: kspaceFirstOrder-openmp-linux
            kind: openmp
          - os: windows-latest
            project: kspaceFirstOrder-openmp-windows
            kind: openmp
          - os: macos-latest
            project: kspaceFirstOrder-openmp-darwin
            kind: openmp
          - os: ubuntu-latest
            project: kspaceFirstOrder-cuda-linux
            kind: cuda
          - os: windows-latest
            project: kspaceFirstOrder-cuda-windows
            kind: cuda
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: true

      - name: Configure (OpenMP)
        if: matrix.kind == 'openmp'
        run: cmake -S repos/${{ matrix.project }} -B build/${{ matrix.project }} -DCMAKE_BUILD_TYPE=Release -DUSE_CUDA=OFF -DUSE_OPENMP=ON

      - name: Build (OpenMP)
        if: matrix.kind == 'openmp'
        run: cmake --build build/${{ matrix.project }} --config Release --parallel

      - name: Install CUDA Toolkit (Linux)
        if: matrix.kind == 'cuda' && runner.os == 'Linux'
        uses: N-Storm/cuda-toolkit@v0.2.27m
        with:
          cuda: '13.0.0'
          use-github-cache: false

      - name: Install CUDA Toolkit (Windows)
        if: matrix.kind == 'cuda' && runner.os == 'Windows'
        shell: bash
        run: |
          set -e
          curl -Lo cuda_13.0.0_windows_network.exe https://developer.download.nvidia.com/compute/cuda/13.0.0/network_installers/cuda_13.0.0_windows_network.exe
          powershell -Command "Start-Process -FilePath cuda_13.0.0_windows_network.exe -ArgumentList '-s','-n' -Wait"
          rm -f ./cuda_13.0.0_windows_network.exe
          echo "CUDA_PATH=C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v13.0" >> $GITHUB_ENV
          echo "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v13.0\\bin" >> $GITHUB_PATH

      - name: Configure (CUDA, compile-only)
        if: matrix.kind == 'cuda'
        run: cmake -S repos/${{ matrix.project }} -B build/${{ matrix.project }} -DCMAKE_BUILD_TYPE=Release -DUSE_CUDA=ON -DUSE_OPENMP=OFF

      - name: Build (CUDA, compile-only)
        if: matrix.kind == 'cuda'
        run: cmake --build build/${{ matrix.project }} --config Release --parallel

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: ${{ matrix.project }}-${{ matrix.os }}
          path: build/${{ matrix.project }}/
```

Acceptance:
- Each project builds on its target OS only, uploading artifacts.
- No changes to existing workflows (`ci.yml`, static analysis, format) yet.

Stage gates:
- 2A: Linux build for `kspaceFirstOrder-openmp-linux` green.
- 2B: Add Windows and macOS OpenMP builds green.
- 2C: Add Linux CUDA compile-only green.
- 2D: Add Windows CUDA compile-only green.

What could go wrong (and mitigations):
- Upstream not using CMake → Use native build (Make/MSBuild); if too complex, add a wrapper CMakeLists in `wrappers/<project>/` that calls the native build with `execute_process`.
- Missing dependencies (FFTW/HDF5) → Start with CPU/OpenMP configuration and disable optional features; integrate vcpkg later in Step 4.
- CUDA toolchain instability on CI → Defer CUDA to stage B; for `kspaceFirstOrder-CUDA`, add a “configure-only” or “headers-only” check first.
- MATLAB runner requires license → Skip MATLAB execution; treat as packaging only until we can integrate MathWorks Actions.
- Windows/MSVC OpenMP quirks → Limit to Linux first; enable macOS/Windows after green Linux baseline.
- macOS OpenMP unavailable in AppleClang → Install Homebrew `llvm` and set `OpenMP` flags/env; or build without OpenMP initially on macOS.

## Step 3 — Basic Smoke Tests
Actions:
- For each project, add a lightweight `--version`/`--help` invocation post-build (or a tiny test) to ensure runtime viability on CI.

Acceptance:
- CI runs binaries for each project without crashing; artifacts still upload.

What could go wrong (and mitigations):
- No `--version/--help` implemented → Use `--help` if available; otherwise run a no-op command or a tiny validation test packaged with the repo.
- Binaries require input files to start → Use argument that avoids file I/O; otherwise, add a minimal synthetic HDF5 input produced during CI.
- GPU runtime not available → Don’t run GPU binaries; limit to compilation checks for CUDA.

## Step 4 — Stabilize Dependencies (optional early step)
Actions:
- If subprojects already use vcpkg manifests, allow vcpkg bootstrap in the start-over workflow.
- Otherwise, keep builds minimal (OpenMP-only or CPU-only). Defer CUDA/HDF5/FFTW until Linux-only baseline is green.

Acceptance:
- Builds remain green across Linux/macOS/Windows for the minimal feature set.

What could go wrong (and mitigations):
- Conflicts between upstream Find modules and vcpkg configs → Prefer toolchain isolation per project; pass `CMAKE_TOOLCHAIN_FILE` only when the project supports it.
- Static vs shared linking differences across OSes → Normalize with `BUILD_SHARED_LIBS` and consistent `CMAKE_MSVC_RUNTIME_LIBRARY` on Windows.
- FFTW/HDF5 ABI mismatches → Pin versions via vcpkg manifest when enabling; otherwise, use system packages on Linux only.

## Step 5 — First Consolidation Cut (post-baseline)
Actions:
- Identify duplicated utilities (logging, params, HDF5 wrappers, FFT layers, dimension types, SIMD helpers). Propose a new shared `core/` library in this repo.
- Create a staging plan to:
  - Extract one small, low-risk common component into `src/core/` with clean API.
  - Patch exactly one subproject to optionally consume `core` via `add_subdirectory(..)` or an interface target.
  - Keep submodules intact; do not remove original code yet.

Acceptance:
- One subproject successfully builds against the shared `core` library on all OSes.

What could go wrong (and mitigations):
- License compatibility for code moved into `core/` → Confirm licenses and attribution; keep provenance notes.
- Header/API conflicts with upstream code → Use namespaced headers (`core/…`) and unique target names; prefer interface targets for headers-only utilities.
- Hidden coupling in upstream implementations → Start with leaf utilities (e.g., `DimensionSizes`, logging macros) before touching heavy components (FFT/HDF5 backends).

## Step 6 — Progressive Migration
Actions:
- Repeat extraction for other shared pieces; move more subprojects to the shared `core` where feasible.
- Introduce a top-level superbuild CMake (adds subdirectories or `ExternalProject`) only when at least two subprojects share `core`.
- Gate CUDA/HDF5/FFTW features behind consistent CMake options, using presets.

Acceptance:
- Two or more subprojects build and link against shared `core` in CI.

## Step 7 — Unification and Submodule Retirement
Actions:
- When a subproject’s code is mostly replaced by shared `core` + thin glue, copy/merge the remaining unique code into this repo and remove the submodule.
- Preserve history via subtree import or note provenance in docs/releases.

Acceptance:
- Submodule count decreases with each successful merge; CI remains green.

## Risks and Rollback
- Any subproject can be pinned to a previous SHA if a new upstream change breaks CI.
- The start-over workflow is additive; removing it reverts us to the current state.
- Keep merges small; if a consolidation PR causes risk, revert quickly and retry with a smaller slice.

## Immediate Next PRs
1) 06a-inventory.md with repo list, URLs, SHAs, minimal build instructions.
2) Add five submodules under `repos/` pinned to SHAs.
3) Add `ci-start-over.yml` for Linux-only build matrix.
4) Add `--help/--version` smoke steps and expand matrix to macOS/Windows.

Once the above are green, we begin the careful, iterative consolidation into a single simpler CMake project.
