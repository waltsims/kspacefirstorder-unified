# CPP Unification Roadmap

## 1. Stabilize Current Baseline
- Merge recent Windows OpenMP fixes into `main`.
- Tag the repo or document exact revisions/steps that yield a clean build.
- Snapshot current vcpkg commit and MSVC version for traceability.

## 2. Add vcpkg Manifest + Superbuild Skeleton
- Create `vcpkg.json` enumerating all dependencies (fftw3, hdf5[szip], libaec, zlib, etc.).
- Add a top-level CMake project that orchestrates builds for each submodule (even if they still use legacy build systems).
- Update developer docs/CI to use the manifest-based workflow (`vcpkg install --triplet x64-windows-static`, etc.).

## 3. Port Submodules to CMake Incrementally
- For each `repos/*` project:
  - Mirror existing compiler flags and options in a new `CMakeLists.txt`.
  - Rely on vcpkg exports (`find_package(FFTW3 CONFIG REQUIRED)`, `libaec::aec`, `hdf5::hdf5`, etc.).
  - Validate on Windows, macOS, Linux; retire old MSBuild/Makefiles once parity is confirmed.
- Update CI to call `cmake -S . -B build -G Ninja` and `cmake --build build` per project.

## 4. Harmonize Configuration Across Repos
- Catalog compiler warnings, options, optional features (OpenMP, CUDA), and platform-specific code.
- Standardize common options (e.g., `KWAVE_ENABLE_CUDA`, `KWAVE_ENABLE_OPENMP`).
- Introduce shared CMake modules/toolchain files for consistent flags and dependency handling.

## 5. Plan the Unified Codebase
- With all projects on CMake, analyze overlaps vs. unique code paths (CPU vs CUDA kernels, file I/O, logging, parameter parsing, etc.).
- Draft an architecture where:
  - A shared core library houses common simulation logic.
  - Platform/accelerator-specific adapters (OpenMP, CUDA) plug into the core.
  - Optional components are toggled via CMake options.
- Document required refactors or rewrites in the plan before executing.

## 6. Execute Unification
- Refactor duplicated code into the shared core module.
- Maintain submodule builds while migrating, then deprecate once the unified tree covers their functionality.
- Ensure CI has a matrix build (Windows/macOS/Linux; CPU/CUDA variants). Add unit/integration tests via CTest.

## 7. Quality & Packaging Enhancements
- Introduce `CMakePresets.json` for standard build configurations.
- Wire in clang-format/clang-tidy targets for consistency.
- Add tests (GoogleTest/Catch2) and wire into CTest.
- Consider CPack/installer/binary packaging once the unified build is stable.

## 8. Documentation & Developer Experience
- Refresh README / CONTRIBUTING with the new build procedure.
- Provide scripts or Make targets (`cmake --build build --target vcpkg-bootstrap`) to simplify onboarding.
- Summarize the architecture/unification decisions in the docs for historical context.

---
**Outcome**: A single CMake/vcpkg-based k-Wave codebase that builds reproducibly across Windows, Linux, and macOS, with shared logic for CPU/OpenMP/CUDA variants and modern CI/test coverage.
