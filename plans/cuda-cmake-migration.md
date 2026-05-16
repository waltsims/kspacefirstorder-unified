# CUDA CMake Migration Plan

## 1. Make-Based Build Inventory (Ubuntu)
- **Primary target**: `kspaceFirstOrder-CUDA` executable compiled with `nvcc`.
- **Source layout** (relative to repo root):
  - C++: `main.cpp`, `Containers/*.cpp`, `Hdf5/*.cpp`, `Logger/Logger.cpp`, `MatrixClasses/*.cpp`, `OutputStreams/*.cpp`, `Parameters/*.cpp`, `GetoptWin64/Getopt.cpp`.
  - CUDA: `Containers/CudaMatrixContainer.cu`, `KSpaceSolver/SolverCudaKernels.cu`, `MatrixClasses/TransposeCudaKernels.cu`, `OutputStreams/OutputStreamsCudaKernels.cu`, `Parameters/CudaDeviceConstants.cu`.
  - Headers expect include root at repository top; CUDA headers sit alongside the `.cu` files plus `Utils/CudaUtils.cuh`.
- **Compiler toolchain**: `nvcc` for both `.cu` and `.cpp`. Host compiler toggled between GCC and Intel via `COMPILER`.
- **Key flags**:
  - Host: `-std=c++11`, `-O3`, `-ffast-math`, `-fassociative-math`, `-fopenmp` (GCC) or `-qopenmp` (Intel), CPU ISA switch (`-march=native` default, alternatives `-mavx`, `-mavx2`, `-mavx512f` or Intel equivalents).
  - Device: `-O3`, `--device-c`, `--restrict`, CUDA architectures list covering `compute_50` through `sm_90a`.
  - Definition: `-D__KWAVE_GIT_HASH__="468dc31c2842a7df5f2a07c3a13c16c9b0b2b770"`.
- **Libraries**:
  - HDF5 core + high level (`libhdf5`, `libhdf5_hl`), zlib (`-lz`), szip (`-lsz`), CUDA runtime (`libcudart`), cuFFT (`libcufft`), `-ldl`; optional static CUDA libs when `LINKING=STATIC`.
  - Include directories derived from `HDF5_DIR` and `CUDA_DIR`.
- **Environment variables**: defaults read from `CUDA_HOME`, `EBROOTHDF5`, `EBROOTZLIB`, `EBROOTSZIP`.
- **OpenMP**: optional via `_OPENMP`; affects usage text and thread defaults.
- **Build artefacts**: single executable; `clean` target removes objects + binary.

## 2. Target CMake Architecture
- **Top-level (`CMakeLists.txt`)**:
  - `cmake_minimum_required(VERSION 3.24)` to gain modern CUDA handling.
  - `project(kspaceFirstOrderCUDA LANGUAGES CXX CUDA)`.
  - Options:
    - `KSPACE_CPU_ARCH` (string cache; values `native|avx|avx2|avx512`).
    - `KSPACE_ENABLE_OPENMP` (default `ON`).
    - `KSPACE_ENABLE_FAST_MATH` (default `ON` to mirror `-ffast-math` behaviour).
    - `KSPACE_GIT_HASH` (string, default populated from `git describe --always` fallback to legacy value).
    - `KSPACE_CUDA_ARCH_LIST` (semicolon list; default matches Makefile set, but allow override).
  - Detect packages: `find_package(CUDAToolkit REQUIRED)`, `find_package(HDF5 REQUIRED COMPONENTS C HL)`, `find_package(OpenMP)` gated by option, and fall back to `find_package(ZLIB REQUIRED)` / `find_package(SZIP)` if HDF5 does not propagate them.
  - Set policies for CUDA (`CMP0104`, `CMP0146`) to ensure separable compilation and architecture configuration work as intended.
- **Source organisation**:
  - `set(KSPACE_CXX_SOURCES ...)` grouping `.cpp` files.
  - `set(KSPACE_CUDA_SOURCES ...)` grouping `.cu` files.
  - `add_executable(kspaceFirstOrder-CUDA ${KSPACE_CXX_SOURCES} ${KSPACE_CUDA_SOURCES})`.
  - Promote repository root include path to target via `target_include_directories` (PRIVATE).
  - Enable separable compilation: `set_target_properties(... PROPERTIES CUDA_SEPARABLE_COMPILATION ON)`.
- **Compiler / linker flags**:
  - CXX language block: inject CPU ISA (`-march=native` or alternatives), optimisation, fast-math toggles.
  - CUDA language block: `--restrict`, `--use_fast_math` (when requested), and host-inherited CPU ISA via `-Xcompiler`.
  - Configure `CMAKE_CUDA_ARCHITECTURES` from `KSPACE_CUDA_ARCH_LIST`.
  - Apply `_OPENMP` define when OpenMP is enabled; attach `OpenMP::OpenMP_CXX` to the target.
  - Add compile definitions: `__KWAVE_GIT_HASH__="<value>"`.
- **Linking**:
  - `target_link_libraries(kspaceFirstOrder-CUDA PRIVATE CUDA::cudart CUDA::cufft HDF5::HDF5 HDF5::HL ZLIB::ZLIB dl)` and conditionally `SZIP::SZIP` if available.
  - Use `target_link_directories` sparingly; prefer imported targets from find modules.
- **Utilities**:
  - Optional `scripts/cuda-arch.cmake` helper to validate `KSPACE_CUDA_ARCH_LIST`.
  - Add `cmake/Modules` folder if custom find scripts or arch parsing is needed.

## 3. Migration Task Breakdown
1. **Bootstrap CMake files**  
   - Author top-level `CMakeLists.txt` per architecture above; confirm source lists match Makefile dependencies.  
   - Introduce `cmake/` helpers if required (e.g., git hash capture via `execute_process` guarded for offline runs).

2. **Configure options and dependency discovery**  
   - Implement options, map CPU arch strings to compiler flags, and surface helpful cache messages.  
   - Validate `find_package` calls on Ubuntu with CUDA Toolkit (prefer `/usr/local/cuda` default fallbacks).  
   - Ensure HDF5 linkage resolves both static and dynamic library scenarios; fall back to user-supplied `HDF5_ROOT`.

3. **Port host/device compilation specifics**  
   - Mirror optimisation, fast-math, and restrict semantics; enable separable compilation; propagate `_GNU_SOURCE` if needed.  
   - Handle OpenMP detection gracefully (warning if requested but missing).

4. **Update tooling**  
   - Add `build` instructions to repository README; provide sample configure command (`cmake -S . -B build -DKSPACE_CPU_ARCH=native`).  
   - Add Ubuntu CI job invoking `cmake --build` and `ctest` (if/when tests exist); keep Makefile build until parity confirmed.

5. **Validation & parity**  
   - Build on Ubuntu with supported CUDA toolkit; compare binary size and `nvdisasm --print-code` for kernel presence.  
   - Run existing smoke tests (`--help`, dry-run) to ensure runtime equivalence before retiring Make build.

## 4. Makefile Features → CMake Mapping (Work in Progress)
- **Compiler/flags**: Host ISA/optimization (`CPU_ARCH`, `CPU_OPT`, fast-math) mapped via `KSPACE_CPU_ARCH` and `KSPACE_ENABLE_FAST_MATH`; device uses `--restrict`, `--device-c`, and inherited host flags.
- **CUDA architectures**: Default covers modern toolkits (`75+`); override via `KSPACE_CUDA_ARCH_LIST` to target legacy GPUs.
- **SetupCUDA integration**: `cmake/SetupCUDA.cmake` (from NVIDIA) normalizes architecture lists and enables CUDA language before target creation.
- **Git hash define**: `GIT_HASH` replaced with cache variable `KSPACE_GIT_HASH` -> compile definition `__KWAVE_GIT_HASH__`.
- **Dependencies**: Manual include/lib paths swapped for `find_package` calls: `CUDAToolkit`, `HDF5`, `ZLIB`, optional `SZIP`, and OpenMP when enabled (require official `hdf5::hdf5_hl` target).
- **Linking semantics**: Currently defaults to SEMI (dynamic CUDA, static HDF5) behaviour through imported targets; explicit STATIC/DYNAMIC switches still TODO.
- **Ancillary sources**: Linux build omits `GetoptWin64` as in Makefile; `.cu`/`.cpp` lists mirror `DEPENDENCIES`.
- **Rpath handling**: Makefile `-Xlinker -rpath` intentionally deferred; add once install/runtime layout is defined.
- **cuFFT error handling**: Conditional maps for CUDA 13 vs earlier toolkits keep compilation portable.
