######################################################################
# HDF5 (required)
######################################################################

set(HDF5_PREFER_PARALLEL OFF CACHE BOOL "Prefer parallel HDF5 (MPI)")

find_package(HDF5 CONFIG REQUIRED COMPONENTS C HL)

add_library(kspace_hdf5 INTERFACE)

if(TARGET hdf5::hdf5)
    target_link_libraries(kspace_hdf5 INTERFACE hdf5::hdf5)
elseif(TARGET hdf5::hdf5-shared)
    target_link_libraries(kspace_hdf5 INTERFACE hdf5::hdf5-shared)
elseif(TARGET hdf5::hdf5-static)
    target_link_libraries(kspace_hdf5 INTERFACE hdf5::hdf5-static)
else()
    message(FATAL_ERROR "HDF5 config package found but core imported target not present")
endif()

if(TARGET hdf5::hdf5_hl)
    target_link_libraries(kspace_hdf5 INTERFACE hdf5::hdf5_hl)
elseif(TARGET hdf5::hdf5_hl-shared)
    target_link_libraries(kspace_hdf5 INTERFACE hdf5::hdf5_hl-shared)
elseif(TARGET hdf5::hdf5_hl-static)
    target_link_libraries(kspace_hdf5 INTERFACE hdf5::hdf5_hl-static)
endif()

# FFTW3 (for OpenMP backend, required when USE_OPENMP)
if(USE_OPENMP)
    find_package(FFTW3 CONFIG REQUIRED)
    if(NOT TARGET FFTW3::fftw3f)
        message(FATAL_ERROR "FFTW3::fftw3f target not found (require single-precision)")
    endif()
    if(NOT TARGET FFTW3::fftw3f_threads)
        message(FATAL_ERROR "FFTW3::fftw3f_threads target not found (require threads)")
    endif()
endif()

# CUDA (for CUDA backend)
if(USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "✓ Found CUDA Toolkit ${CUDAToolkit_VERSION}")
    # Set CUDA architectures - support from Kepler to Hopper
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 50 52 53 60 61 62 70 72 75 80 87 89 90 90a)
    endif()
    message(STATUS "  - CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")
endif()

# OpenMP
if(USE_OPENMP)
    find_package(OpenMP REQUIRED)
    if(OpenMP_FOUND)
        message(STATUS "✓ Found OpenMP ${OpenMP_CXX_VERSION}")
    endif()
endif()
