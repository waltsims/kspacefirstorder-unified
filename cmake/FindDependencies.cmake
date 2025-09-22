# Find HDF5
find_package(HDF5 COMPONENTS C CXX HL REQUIRED)
if(NOT HDF5_FOUND)
    message(FATAL_ERROR "HDF5 not found. Please install HDF5 with C++ support (see DEPENDENCIES.md)")
endif()

# Ensure high-level (H5LT) libs are linked
if(DEFINED HDF5_HL_LIBRARIES)
    list(APPEND HDF5_LIBRARIES ${HDF5_HL_LIBRARIES})
endif()

message(STATUS "✓ Found HDF5 ${HDF5_VERSION}")
message(STATUS "  - Include dirs: ${HDF5_INCLUDE_DIRS}")
message(STATUS "  - Libraries: ${HDF5_LIBRARIES}")

# Find FFTW3 (for OpenMP backend)
if(USE_OPENMP)
    # Define search paths
    set(FFTW_SEARCH_PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /opt/homebrew/lib
        $ENV{LOCALAPPDATA}/fftw
        ${CMAKE_SOURCE_DIR}/third-party/fftw
    )
    set(FFTW_INCLUDE_SEARCH_PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        /opt/homebrew/include
        $ENV{LOCALAPPDATA}/fftw/include
        ${CMAKE_SOURCE_DIR}/third-party/fftw/include
    )

    find_library(FFTW_LIBRARY
        NAMES fftw3f libfftw3f
        PATHS ${FFTW_SEARCH_PATHS}
    )
    # Threads library for fftwf_init_threads/plan_with_nthreads
    find_library(FFTW_THREADS_LIBRARY
        NAMES fftw3f_threads libfftw3f_threads
        PATHS ${FFTW_SEARCH_PATHS}
    )
    find_path(FFTW_INCLUDE_DIR
        NAMES fftw3.h
        PATHS ${FFTW_INCLUDE_SEARCH_PATHS}
    )
    if(FFTW_LIBRARY AND FFTW_INCLUDE_DIR)
        set(FFTW_FOUND TRUE)
        set(FFTW_LIBRARIES ${FFTW_LIBRARY})
        if(FFTW_THREADS_LIBRARY)
            list(APPEND FFTW_LIBRARIES ${FFTW_THREADS_LIBRARY})
        endif()
        set(FFTW_INCLUDE_DIRS ${FFTW_INCLUDE_DIR})
    endif()

    if(NOT FFTW_FOUND)
        message(FATAL_ERROR "FFTW3 (single precision) not found. Please install FFTW3 (see DEPENDENCIES.md)")
    endif()

    message(STATUS "✓ Found FFTW3 (single precision)")
    message(STATUS "  - Include dirs: ${FFTW_INCLUDE_DIRS}")
    message(STATUS "  - Libraries: ${FFTW_LIBRARIES}")
endif()

# Find CUDA (for CUDA backend)
if(USE_CUDA)
    find_package(CUDAToolkit QUIET)
    if(NOT CUDAToolkit_FOUND)
        message(WARNING "CUDA Toolkit not found - CUDA backend will be disabled")
        message(WARNING "Please install CUDA Toolkit to enable CUDA backend (see DEPENDENCIES.md)")
        set(USE_CUDA OFF CACHE BOOL "" FORCE)
    else()
        message(STATUS "✓ Found CUDA Toolkit ${CUDAToolkit_VERSION}")
        # Set CUDA architectures - support from Kepler to Hopper
        set(CMAKE_CUDA_ARCHITECTURES 50 52 53 60 61 62 70 72 75 80 87 89 90 90a)
        message(STATUS "  - CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")
    endif()
endif()

# Find OpenMP
if(USE_OPENMP)
    find_package(OpenMP REQUIRED)
    if(OpenMP_FOUND)
        message(STATUS "✓ Found OpenMP ${OpenMP_CXX_VERSION}")
    endif()
endif()