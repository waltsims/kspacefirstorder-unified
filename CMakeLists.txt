cmake_minimum_required(VERSION 3.18)

# Project configuration
project(kspaceFirstOrder VERSION 3.6 LANGUAGES C CXX)

# Backend options
option(USE_CUDA "Enable CUDA backend" OFF)
option(USE_OPENMP "Enable OpenMP backend" ON)
option(BUILD_TESTS "Build test suite" OFF)
option(ENABLE_NATIVE_ARCH "Enable march=native for performance" ON)

# Add CUDA language if needed
if(USE_CUDA)
    enable_language(CUDA)
endif()

# Validate backend options
if(USE_CUDA AND USE_OPENMP)
    message(FATAL_ERROR "Cannot enable both CUDA and OpenMP backends simultaneously")
elseif(NOT USE_CUDA AND NOT USE_OPENMP)
    message(FATAL_ERROR "Must enable at least one backend (CUDA or OpenMP)")
endif()

# Enable compiler cache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "✓ Found ccache, enabling compiler cache")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    if(USE_CUDA)
        set(CMAKE_CUDA_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    endif()
endif()

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Core configuration
include(PlatformDetection)
include(CompilerFlags)
include(BuildConfig)
include(FindDependencies)

# Build targets
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
