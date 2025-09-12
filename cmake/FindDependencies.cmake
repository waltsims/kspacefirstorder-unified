include(FetchContent)

# HDF5 Dependency Resolution
message(STATUS "Fetching HDF5 from source...")
FetchContent_Declare(
    hdf5
    GIT_REPOSITORY https://github.com/HDFGroup/hdf5.git
    GIT_TAG        hdf5_1_14_4
    GIT_SHALLOW    TRUE
)

set(HDF5_BUILD_EXAMPLES OFF CACHE BOOL "")
set(HDF5_BUILD_TOOLS OFF CACHE BOOL "")
set(HDF5_BUILD_TESTS OFF CACHE BOOL "")
set(HDF5_BUILD_UTILS OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(HDF5_ENABLE_PARALLEL OFF CACHE BOOL "")
set(HDF5_BUILD_HL_LIB ON CACHE BOOL "")

# Suppress warnings from HDF5 build on macOS
if(APPLE)
    set(HDF5_ENABLE_WARNINGS OFF CACHE BOOL "")
    # Save current flags
    set(SAVED_C_FLAGS "${CMAKE_C_FLAGS}")
    set(SAVED_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    # Temporarily add warning suppressions for HDF5 build
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-redundant-decls -Wno-pedantic -w")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-redundant-decls -Wno-pedantic -w")
endif()

FetchContent_MakeAvailable(hdf5)

# Restore original compiler flags on macOS
if(APPLE)
    set(CMAKE_C_FLAGS "${SAVED_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${SAVED_CXX_FLAGS}")
endif()

# Set HDF5 variables for compatibility
set(HDF5_FOUND TRUE)
set(HDF5_LIBRARIES hdf5-static hdf5_hl-static)
set(HDF5_INCLUDE_DIRS ${hdf5_SOURCE_DIR}/src ${hdf5_BINARY_DIR} ${hdf5_BINARY_DIR}/src)
message(STATUS "✓ Built HDF5 from source")


# FFTW3 Dependency Resolution (for OpenMP backend)
if(USE_OPENMP)
    message(STATUS "Fetching FFTW3 from source...")
    FetchContent_Declare(
        fftw3
        URL https://www.fftw.org/fftw-3.3.10.tar.gz
        URL_HASH SHA256=56c932549852cddcfafdab3820b0200c7742675be92179e59e6215b340e26467
    )

    # Configure FFTW3 for single-precision (required by kspaceFirstOrder)
    set(ENABLE_FLOAT ON CACHE BOOL "")
    set(BUILD_TESTS OFF CACHE BOOL "")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
    
    # Suppress warnings from FFTW build on macOS
    if(APPLE)
        # Save current flags
        set(SAVED_C_FLAGS_FFTW "${CMAKE_C_FLAGS}")
        set(SAVED_CXX_FLAGS_FFTW "${CMAKE_CXX_FLAGS}")
        # Temporarily add warning suppressions for FFTW build
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    endif()
    
    # The FFTW 3.3.10 tarball uses a very old CMake version.
    # By setting this policy globally before making the content available,
    # we allow our modern CMake to build it.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_MakeAvailable(fftw3)
    # Unset the policy to avoid affecting other parts of the build
    unset(CMAKE_POLICY_VERSION_MINIMUM)
    
    # Restore original compiler flags on macOS
    if(APPLE)
        set(CMAKE_C_FLAGS "${SAVED_C_FLAGS_FFTW}")
        set(CMAKE_CXX_FLAGS "${SAVED_CXX_FLAGS_FFTW}")
    endif()

    set(FFTW_FOUND TRUE)
    set(FFTW_LIBRARIES fftw3f)
    set(FFTW_INCLUDE_DIRS ${fftw3_SOURCE_DIR}/api)
    message(STATUS "✓ Built FFTW3 from source (single-precision)")
endif()

# Find CUDA (for CUDA backend)
if(USE_CUDA)
    find_package(CUDAToolkit QUIET)
    if(NOT CUDAToolkit_FOUND)
        message(WARNING "CUDA Toolkit not found - CUDA backend disabled")
        set(USE_CUDA OFF)
    else()
        # Set CUDA architectures
        set(CMAKE_CUDA_ARCHITECTURES 50 52 53 60 61 62 70 72 75 80 87 89 90 90a)
    endif()
endif()

# Find OpenMP
if(USE_OPENMP)
    find_package(OpenMP QUIET)
    # Create a resilient INTERFACE target for OpenMP. This allows us to handle
    # the case where the OpenMP package is not found (e.g., on default macOS).
    add_library(kspace_openmp_flags INTERFACE)
    if(OpenMP_FOUND)
        message(STATUS "✓ Found OpenMP, linking against OpenMP::OpenMP_CXX")
        target_link_libraries(kspace_openmp_flags INTERFACE OpenMP::OpenMP_CXX)
    else()
        # Check if we're using Apple Clang which doesn't support -fopenmp
        if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
            message(WARNING "Apple Clang detected - OpenMP not supported. Consider using Homebrew GCC:")
            message(WARNING "  CC=gcc-15 CXX=g++-15 cmake ..")
            message(WARNING "OpenMP backend will be disabled.")
            set(USE_OPENMP OFF CACHE BOOL "" FORCE)
        else()
            message(WARNING "OpenMP package not found. Attempting to enable with -fopenmp flag.")
            # For GCC and non-Apple Clang, -fopenmp is typically sufficient for both compiling and linking.
            target_compile_options(kspace_openmp_flags INTERFACE -fopenmp)
            target_link_libraries(kspace_openmp_flags INTERFACE -fopenmp)
        endif()
    endif()
endif()
