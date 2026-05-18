# AI-Interpretable kspaceFirstOrder Project Unification Plan

## EXECUTIVE SUMMARY

This document provides a step-by-step, AI-executable plan to unify 5 separate kspaceFirstOrder project variants into a single, unified codebase with CMake build system and GitHub Actions CI/CD. The plan is designed to be followed sequentially by an AI agent.

## CURRENT STATE ANALYSIS

### Repository Structure Overview
- **k-wave-omp-darwin**: OpenMP version for macOS (Homebrew paths)
- **kspaceFirstOrder-CUDA-linux**: CUDA version for Linux
- **kspaceFirstOrder-CUDA-windows**: CUDA version for Windows
- **kspaceFirstOrder-OMP-linux**: OpenMP version for Linux
- **kspaceFirstOrder-OMP-windows**: OpenMP version for Windows

### Key Differences Identified

#### 1. Build Systems
- **Linux/macOS OpenMP**: Makefile with hardcoded paths
- **Linux CUDA**: Complex Makefile with nvcc compilation
- **Windows**: Visual Studio .vcxproj files

#### 2. Backend-Specific Files

**CUDA-specific files (present in CUDA variants only):**
- `Containers/CudaMatrixContainer.cu`, `CudaMatrixContainer.cuh`
- `KSpaceSolver/SolverCudaKernels.cu`, `SolverCudaKernels.cuh`
- `MatrixClasses/CufftComplexMatrix.cpp`, `CufftComplexMatrix.h`
- `MatrixClasses/TransposeCudaKernels.cu`, `TransposeCudaKernels.cuh`
- `OutputStreams/OutputStreamsCudaKernels.cu`, `OutputStreamsCudaKernels.cuh`
- `Parameters/CudaParameters.cpp`, `CudaParameters.h`
- `Parameters/CudaDeviceConstants.cu`, `CudaDeviceConstants.cuh`
- `Utils/CudaUtils.cuh`

**OpenMP-specific files (present in OpenMP variants only):**
- `MatrixClasses/FftwComplexMatrix.cpp`, `FftwComplexMatrix.h`
- `MatrixClasses/FftwRealMatrix.cpp`, `FftwRealMatrix.h`

#### 3. Platform-Specific Files
**All variants contain:**
- `Logger/ErrorMessagesLinux.h`, `ErrorMessagesWindows.h`
- `Logger/OutputMessagesLinux.h`, `OutputMessagesWindows.h`

#### 4. Common Files (present in all variants)
- `main.cpp`
- `Containers/MatrixContainer.cpp`, `MatrixContainer.h`
- `Containers/MatrixRecord.h`
- `Containers/OutputStreamContainer.cpp`, `OutputStreamContainer.h`
- `Hdf5/Hdf5File.cpp`, `Hdf5File.h`, `Hdf5FileHeader.cpp`, `Hdf5FileHeader.h`
- `KSpaceSolver/KSpaceFirstOrderSolver.cpp`, `KSpaceFirstOrderSolver.h`
- `Logger/Logger.cpp`, `Logger.h`, `ErrorMessages.h`, `OutputMessages.h`
- `MatrixClasses/BaseFloatMatrix.cpp`, `BaseFloatMatrix.h`
- `MatrixClasses/BaseIndexMatrix.cpp`, `BaseIndexMatrix.h`
- `MatrixClasses/BaseMatrix.h`
- `MatrixClasses/ComplexMatrix.cpp`, `ComplexMatrix.h`
- `MatrixClasses/IndexMatrix.cpp`, `IndexMatrix.h`
- `MatrixClasses/RealMatrix.cpp`, `RealMatrix.h`
- `OutputStreams/BaseOutputStream.cpp`, `BaseOutputStream.h`
- `OutputStreams/CuboidOutputStream.cpp`, `CuboidOutputStream.h`
- `OutputStreams/IndexOutputStream.cpp`, `IndexOutputStream.h`
- `OutputStreams/WholeDomainOutputStream.cpp`, `WholeDomainOutputStream.h`
- `Parameters/CommandLineParameters.cpp`, `CommandLineParameters.h`
- `Parameters/Parameters.cpp`, `Parameters.h`
- `Utils/DimensionSizes.h`, `TimeMeasure.h`
- Documentation files: `Readme.md`, `License.md`, `Changelog.md`, `Doxyfile`
- Build artifacts: `Makefile`, `*.vcxproj`, `header_bg.png`

## UNIFICATION STRATEGY

### Approach
1. **Conservative Unification**: Preserve all functionality while eliminating duplication
2. **Conditional Compilation**: Use CMake to enable/disable CUDA vs OpenMP features
3. **Platform Abstraction**: Abstract platform-specific differences
4. **Gradual Migration**: Maintain buildability of existing variants during transition

## STEP-BY-STEP UNIFICATION PLAN

### PHASE 1: PROJECT STRUCTURE SETUP

#### Step 1.1: Create Unified Directory Structure
```
ACTION: Create the following directory structure at the root level
TARGET: /Users/walters/git/kspaceFirstOrder-unified/

kspaceFirstOrder-unified/
├── CMakeLists.txt                    # Main CMake configuration
├── cmake/                           # CMake modules
│   ├── FindDependencies.cmake
│   ├── CompilerFlags.cmake
│   └── PlatformDetection.cmake
├── src/                             # Unified source code
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── core/                        # Platform/backend agnostic code
│   │   ├── CMakeLists.txt
│   │   ├── Containers/
│   │   ├── HDF5/
│   │   ├── Logger/
│   │   ├── Parameters/
│   │   └── Utils/
│   ├── backends/                    # Backend-specific code
│   │   ├── openmp/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── MatrixClasses/
│   │   │   ├── KSpaceSolver/
│   │   │   └── OutputStreams/
│   │   └── cuda/
│   │       ├── CMakeLists.txt
│   │       ├── MatrixClasses/
│   │       ├── KSpaceSolver/
│   │       └── OutputStreams/
│   └── platforms/                   # Platform-specific code
│       ├── unix/
│       └── windows/
├── tests/                           # Test suite
├── docs/                            # Documentation
├── scripts/                         # Build utilities
├── tools/                           # Development tools
└── .github/                         # GitHub Actions
    └── workflows/
```

#### Step 1.2: Initialize CMake Infrastructure
```
ACTION: Create root CMakeLists.txt
CONTENT:
cmake_minimum_required(VERSION 3.18)
project(kspaceFirstOrder VERSION 3.6 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(USE_CUDA "Enable CUDA backend" ON)
option(USE_OPENMP "Enable OpenMP backend" ON)
option(BUILD_TESTS "Build test suite" OFF)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find dependencies
include(FindDependencies)

# Platform detection
include(PlatformDetection)

# Compiler flags
include(CompilerFlags)

# Add subdirectories
add_subdirectory(src)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

### PHASE 2: CORE CODE MIGRATION

#### Step 2.1: Migrate Common Files to src/core/
```
ACTION: Verify consistency of common files across all variants, then copy to src/core/
VERIFICATION_ACTION: Before copying, run a checksum (e.g., sha256sum) on each "common" file across all 5 variants to ensure they are identical. If any discrepancies are found, they must be manually reviewed and resolved before proceeding.
SOURCE: Use k-wave-omp-darwin as the reference for the copy operation after verification.
FILES_TO_COPY:
- main.cpp
- Containers/MatrixContainer.cpp, MatrixContainer.h, MatrixRecord.h, OutputStreamContainer.cpp, OutputStreamContainer.h
- HDF5/Hdf5File.cpp, Hdf5File.h, Hdf5FileHeader.cpp, Hdf5FileHeader.h
- KSpaceSolver/KSpaceFirstOrderSolver.cpp, KSpaceFirstOrderSolver.h
- Logger/Logger.cpp, Logger.h, ErrorMessages.h, OutputMessages.h
- MatrixClasses/BaseFloatMatrix.cpp, BaseFloatMatrix.h, BaseIndexMatrix.cpp, BaseIndexMatrix.h, BaseMatrix.h
- MatrixClasses/ComplexMatrix.cpp, ComplexMatrix.h, IndexMatrix.cpp, IndexMatrix.h, RealMatrix.cpp, RealMatrix.h
- OutputStreams/BaseOutputStream.cpp, BaseOutputStream.h, CuboidOutputStream.cpp, CuboidOutputStream.h
- OutputStreams/IndexOutputStream.cpp, IndexOutputStream.h, WholeDomainOutputStream.cpp, WholeDomainOutputStream.h
- Parameters/CommandLineParameters.cpp, CommandLineParameters.h, Parameters.cpp, Parameters.h
- Utils/DimensionSizes.h, TimeMeasure.h

VERIFICATION: Ensure all files compile without CUDA/OpenMP specific code
```

#### Step 2.2: Migrate OpenMP-Specific Files
```
ACTION: Copy OpenMP-specific files to src/backends/openmp/
SOURCE: k-wave-omp-darwin (most complete OpenMP implementation)
FILES_TO_COPY:
- MatrixClasses/FftwComplexMatrix.cpp, FftwComplexMatrix.h
- MatrixClasses/FftwRealMatrix.cpp, FftwRealMatrix.h

VERIFICATION: These files should contain no CUDA-specific code
```

#### Step 2.3: Migrate CUDA-Specific Files
```
ACTION: Copy CUDA-specific files to src/backends/cuda/
SOURCE: kspaceFirstOrder-CUDA-linux (most complete CUDA implementation)
FILES_TO_COPY:
- Containers/CudaMatrixContainer.cu, CudaMatrixContainer.cuh
- KSpaceSolver/SolverCudaKernels.cu, SolverCudaKernels.cuh
- MatrixClasses/CufftComplexMatrix.cpp, CufftComplexMatrix.h
- MatrixClasses/TransposeCudaKernels.cu, TransposeCudaKernels.cuh
- OutputStreams/OutputStreamsCudaKernels.cu, OutputStreamsCudaKernels.cuh
- Parameters/CudaParameters.cpp, CudaParameters.h
- Parameters/CudaDeviceConstants.cu, CudaDeviceConstants.cuh
- Utils/CudaUtils.cuh

VERIFICATION: These files should contain CUDA-specific code and __device__ functions
```

#### Step 2.4: Migrate Platform-Specific Files
```
ACTION: Copy platform-specific files to src/platforms/
SOURCE: Any variant (they're identical)
FILES_TO_COPY:
- Logger/ErrorMessagesLinux.h, ErrorMessagesWindows.h
- Logger/OutputMessagesLinux.h, OutputMessagesWindows.h

VERIFICATION: Files exist for both Linux and Windows platforms
```

### PHASE 3: CMAKE BUILD SYSTEM IMPLEMENTATION

#### Step 3.1: Create FindDependencies.cmake
```
ACTION: Create cmake/FindDependencies.cmake
CONTENT:
# Find HDF5
find_package(HDF5 REQUIRED COMPONENTS C HL)
if(HDF5_FOUND)
    include_directories(${HDF5_INCLUDE_DIRS})
    set(HDF5_LIBRARIES ${HDF5_C_LIBRARIES} ${HDF5_C_HL_LIBRARIES})
endif()

# Find FFTW (for OpenMP backend)
if(USE_OPENMP)
    find_package(FFTW QUIET)
    if(NOT FFTW_FOUND)
        message(WARNING "FFTW not found - OpenMP backend will be limited")
    endif()
endif()

# Find CUDA (for CUDA backend)
if(USE_CUDA)
    find_package(CUDA QUIET)
    if(NOT CUDA_FOUND)
        message(WARNING "CUDA not found - CUDA backend disabled")
        set(USE_CUDA OFF)
    else()
        enable_language(CUDA)
        # Set CUDA architectures
        set(CMAKE_CUDA_ARCHITECTURES 50 52 53 60 61 62 70 72 75 80 87 89 90 90a)
    endif()
endif()

# Find OpenMP
if(USE_OPENMP)
    find_package(OpenMP QUIET)
    if(NOT OpenMP_FOUND)
        message(WARNING "OpenMP not found - OpenMP backend disabled")
        set(USE_OPENMP OFF)
    endif()
endif()

# Platform-specific libraries
if(UNIX)
    # Linux/macOS specific
elseif(WIN32)
    # Windows specific
endif()
```

#### Step 3.2: Create PlatformDetection.cmake
```
ACTION: Create cmake/PlatformDetection.cmake
CONTENT:
# Detect operating system
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX ON)
    set(PLATFORM_UNIX ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS ON)
    set(PLATFORM_UNIX ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS ON)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Set platform-specific defines
if(PLATFORM_LINUX)
    add_definitions(-D__PLATFORM_LINUX__)
elseif(PLATFORM_MACOS)
    add_definitions(-D__PLATFORM_MACOS__)
elseif(PLATFORM_WINDOWS)
    add_definitions(-D__PLATFORM_WINDOWS__)
endif()
```

#### Step 3.3: Create CompilerFlags.cmake
```
ACTION: Create cmake/CompilerFlags.cmake
CONTENT:
# Common flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Release optimization
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# CUDA flags
if(USE_CUDA)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3 --restrict")
endif()

# OpenMP flags
if(USE_OPENMP)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopenmp")
    endif()
endif()

# Platform-specific flags
if(PLATFORM_LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
elseif(PLATFORM_MACOS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
elseif(PLATFORM_WINDOWS)
    # Windows-specific flags
endif()
```

#### Step 3.4: Create src/CMakeLists.txt
```
ACTION: Create src/CMakeLists.txt
CONTENT:
# Core library (always built)
add_subdirectory(core)

# Backend-specific libraries
if(USE_CUDA)
    add_subdirectory(backends/cuda)
endif()

if(USE_OPENMP)
    add_subdirectory(backends/openmp)
endif()

# Platform-specific libraries
if(PLATFORM_UNIX)
    add_subdirectory(platforms/unix)
elseif(PLATFORM_WINDOWS)
    add_subdirectory(platforms/windows)
endif()

# Main executable
add_executable(kspaceFirstOrder main.cpp)
target_link_libraries(kspaceFirstOrder
    kspace_core
    $<$<BOOL:${USE_CUDA}>:kspace_cuda>
    $<$<BOOL:${USE_OPENMP}>:kspace_openmp>
    $<$<BOOL:${PLATFORM_UNIX}>:kspace_unix>
    $<$<BOOL:${PLATFORM_WINDOWS}>:kspace_windows>
    ${HDF5_LIBRARIES}
    $<$<BOOL:${USE_CUDA}>:${CUDA_LIBRARIES}>
    $<$<BOOL:${USE_OPENMP}>:${FFTW_LIBRARIES}>
)
```

#### Step 3.5: Create Backend-Specific CMakeLists.txt Files

**For src/backends/cuda/CMakeLists.txt:**
```
# CUDA backend library
if(USE_CUDA)
    # Collect CUDA source files
    set(CUDA_SOURCES
        Containers/CudaMatrixContainer.cu
        KSpaceSolver/SolverCudaKernels.cu
        MatrixClasses/TransposeCudaKernels.cu
        OutputStreams/OutputStreamsCudaKernels.cu
        Parameters/CudaDeviceConstants.cu
    )

    # Collect CUDA header files
    set(CUDA_HEADERS
        Containers/CudaMatrixContainer.cuh
        KSpaceSolver/SolverCudaKernels.cuh
        MatrixClasses/TransposeCudaKernels.cuh
        OutputStreams/OutputStreamsCudaKernels.cuh
        Parameters/CudaDeviceConstants.cuh
        Utils/CudaUtils.cuh
    )

    # Create CUDA library
    add_library(kspace_cuda ${CUDA_SOURCES} ${CUDA_HEADERS})
    set_target_properties(kspace_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
    target_include_directories(kspace_cuda PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
```

**For src/backends/openmp/CMakeLists.txt:**
```
# OpenMP backend library
if(USE_OPENMP)
    # Collect OpenMP source files
    set(OPENMP_SOURCES
        MatrixClasses/FftwComplexMatrix.cpp
        MatrixClasses/FftwRealMatrix.cpp
    )

    # Collect OpenMP header files
    set(OPENMP_HEADERS
        MatrixClasses/FftwComplexMatrix.h
        MatrixClasses/FftwRealMatrix.h
    )

    # Create OpenMP library
    add_library(kspace_openmp ${OPENMP_SOURCES} ${OPENMP_HEADERS})
    target_include_directories(kspace_openmp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
```

### PHASE 4: CONDITIONAL COMPILATION IMPLEMENTATION

#### Step 4.1: Modify Core Files for Conditional Compilation
```
ACTION: Update core files to support conditional compilation
TARGET_FILES:
- KSpaceSolver/KSpaceFirstOrderSolver.cpp
- Parameters/Parameters.cpp
- Logger/Logger.cpp

CHANGES_NEEDED:
1. Add #ifdef USE_CUDA blocks around CUDA-specific includes and code
2. Add #ifdef USE_OPENMP blocks around OpenMP-specific includes and code
3. Use platform detection macros for platform-specific code
4. Include appropriate headers based on available backends

EXAMPLE_CHANGES:
In KSpaceFirstOrderSolver.cpp:
#ifdef USE_CUDA
#include "backends/cuda/KSpaceSolver/SolverCudaKernels.cuh"
#endif

In Parameters.cpp:
#ifdef USE_CUDA
#include "backends/cuda/Parameters/CudaParameters.h"
#endif
```

#### Step 4.2: Update Main Entry Point
```
ACTION: Modify main.cpp for conditional compilation
CHANGES_NEEDED:
1. Add conditional includes for CUDA/OpenMP backends
2. Add runtime backend detection and initialization
3. Update initialization logic to handle different backends

EXAMPLE_CHANGES:
#include <iostream>

// Conditional backend includes
#ifdef USE_CUDA
#include "backends/cuda/Parameters/CudaParameters.h"
#endif

#ifdef USE_OPENMP
#include <omp.h>
#endif

int main(int argc, char** argv) {
    // Backend initialization
    #ifdef USE_CUDA
    // Initialize CUDA if available
    #endif

    #ifdef USE_OPENMP
    // Set OpenMP threads
    #endif

    // Rest of main function...
}
```

### PHASE 5: PLATFORM ABSTRACTION

#### Step 5.1: Handle Platform-Specific Differences
```
ACTION: Use preprocessor directives within the core logic to handle minor platform differences, primarily for logger messages.
DETAILS: Instead of creating a full abstraction layer upfront, we will use conditional compilation (`#ifdef __PLATFORM_LINUX__`, `#elif defined(__PLATFORM_WINDOWS__)`, etc.) directly in the files where platform differences exist (e.g., `Logger.cpp`). This simplifies the initial migration. A more formal abstraction layer can be built later if more significant platform-specific code is required.

EXAMPLE (in Logger.cpp):
#ifdef __PLATFORM_LINUX__
#include "platforms/unix/ErrorMessagesLinux.h"
#elif defined(__PLATFORM_WINDOWS__)
#include "platforms/windows/ErrorMessagesWindows.h"
#endif
```

### PHASE 6: BUILD SYSTEM TESTING

#### Step 6.1: Test OpenMP Backend Only
```
ACTION: Configure and build with OpenMP only
COMMAND: mkdir build && cd build
CMAKE_COMMAND: cmake .. -DUSE_CUDA=OFF -DUSE_OPENMP=ON
BUILD_COMMAND: make -j
TEST_PLATFORMS: Linux, macOS, Windows
```

#### Step 6.2: Test CUDA Backend Only
```
ACTION: Configure and build with CUDA only
COMMAND: mkdir build_cuda && cd build_cuda
CMAKE_COMMAND: cmake .. -DUSE_CUDA=ON -DUSE_OPENMP=OFF
BUILD_COMMAND: make -j
TEST_PLATFORMS: Linux, Windows (with CUDA installed)
```

#### Step 6.3: Test Dual Backend
```
ACTION: Configure and build with both backends
COMMAND: mkdir build_dual && cd build_dual
CMAKE_COMMAND: cmake .. -DUSE_CUDA=ON -DUSE_OPENMP=ON
BUILD_COMMAND: make -j
TEST_PLATFORMS: Linux, Windows (with both CUDA and OpenMP)
```

### PHASE 7: GITHUB ACTIONS CI/CD SETUP

#### Step 7.1: Create CI Workflow for Linux
```
ACTION: Create .github/workflows/ci-linux.yml
CONTENT:
name: CI - Linux

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-openmp:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake libhdf5-dev libfftw3-dev
    - name: Configure CMake (OpenMP)
      run: cmake -B build -DUSE_CUDA=OFF -DUSE_OPENMP=ON
    - name: Build
      run: cmake --build build -j

  build-cuda:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - name: Install CUDA
      uses: Jimver/cuda-toolkit@v0.2.8
      with:
        cuda: '11.8.0'
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake libhdf5-dev
    - name: Configure CMake (CUDA)
      run: cmake -B build -DUSE_CUDA=ON -DUSE_OPENMP=OFF
    - name: Build
      run: cmake --build build -j
```

#### Step 7.2: Create CI Workflow for Windows
```
ACTION: Create .github/workflows/ci-windows.yml
CONTENT:
name: CI - Windows

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-openmp:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v3
    - name: Configure CMake (OpenMP)
      run: cmake -B build -DUSE_CUDA=OFF -DUSE_OPENMP=ON
    - name: Build
      run: cmake --build build --config Release -j

  build-cuda:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v3
    - name: Install CUDA
      uses: Jimver/cuda-toolkit@v0.2.8
      with:
        cuda: '11.8.0'
    - name: Configure CMake (CUDA)
      run: cmake -B build -DUSE_CUDA=ON -DUSE_OPENMP=OFF
    - name: Build
      run: cmake --build build --config Release -j
```

#### Step 7.3: Create Release Workflow
```
ACTION: Create .github/workflows/release.yml
CONTENT:
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  release:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        include:
          - os: ubuntu-latest
            name: linux
          - os: macos-latest
            name: macos
          - os: windows-latest
            name: windows
    steps:
    - uses: actions/checkout@v3
    - name: Build release
      run: |
        mkdir build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        cmake --build . --config Release
    - name: Package
      run: |
        cd build
        cpack
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: kspaceFirstOrder-${{ matrix.name }}
        path: build/*.tar.gz
```

### PHASE 8: MIGRATION AND VALIDATION

#### Step 8.1: Create Migration Scripts
```
ACTION: Create scripts to help migrate from old build systems
FILES_TO_CREATE:
- scripts/migrate-from-makefile.sh
- scripts/migrate-from-vcxproj.sh
- scripts/verify-unified-build.sh
```

#### Step 8.2: Update Documentation
```
ACTION: Update all documentation files
FILES_TO_UPDATE:
- README.md: Update build instructions for CMake
- Changelog.md: Document unification
- docs/: Update API documentation

CHANGES_NEEDED:
1. Replace Makefile instructions with CMake instructions
2. Document backend selection options
3. Update platform-specific build requirements
```

#### Step 8.3: Functional Testing
```
ACTION: Test all build variants and configurations
TEST_MATRIX:
- Linux + OpenMP
- Linux + CUDA
- Linux + Both
- macOS + OpenMP
- Windows + OpenMP
- Windows + CUDA
- Windows + Both

VERIFICATION_STEPS:
1. All variants compile successfully
2. Executables run without errors
3. Basic functionality works (parameter parsing, file I/O)
4. Performance is maintained (compare with original variants)
```

### PHASE 9: CLEANUP AND FINALIZATION

#### Step 9.1: Remove Duplicate Files
```
ACTION: Remove original variant directories after verification
COMMAND: rm -rf k-wave-omp-darwin kspaceFirstOrder-CUDA-* kspaceFirstOrder-OMP-*
TIMING: After successful testing and validation of unified build
```

#### Step 9.2: Update Repository Structure
```
ACTION: Finalize repository structure
ACTIONS:
1. Move documentation files to docs/
2. Move build scripts to scripts/
3. Create .gitignore for unified project
4. Update LICENSE and README
```

#### Step 9.3: Create Backward Compatibility
```
ACTION: Create compatibility layer if needed
FILES_TO_CREATE:
- scripts/legacy-build-wrapper.sh (for old Makefile users)
- docs/migration-guide.md
```

## SUCCESS CRITERIA

### Functional Requirements
- [ ] All original variants build successfully via CMake
- [ ] OpenMP backend works on all platforms
- [ ] CUDA backend works on Linux and Windows
- [ ] Dual backend support (OpenMP + CUDA)
- [ ] Platform-specific features preserved (error messages, paths)

### Performance Requirements
- [ ] No performance regression compared to original variants
- [ ] Memory usage remains consistent
- [ ] Build times are reasonable

### Quality Requirements
- [ ] All CI workflows pass
- [ ] Code compiles without warnings
- [ ] Documentation is updated and accurate
- [ ] Test coverage maintained

## ROLLBACK PLAN

If unification fails at any point:
1. Preserve original variant directories
2. Create separate branch for unified version
3. Allow gradual migration with fallback to original builds
4. Document any breaking changes clearly

## MONITORING AND MAINTENANCE

### Post-Unification Tasks
1. Monitor CI/CD pipelines for failures
2. Update dependencies as needed
3. Add new platform/backend support as required
4. Maintain backward compatibility for existing users

This plan provides a comprehensive, step-by-step approach that an AI can follow to successfully unify the kspaceFirstOrder project while preserving all functionality and maintaining build compatibility across all platforms and backends.
