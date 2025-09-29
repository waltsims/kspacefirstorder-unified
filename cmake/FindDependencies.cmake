# Find HDF5
set(_hdf5_hl_libs "")
set(_hdf5_found_manual OFF)
set(_hdf5_candidate_prefixes "")

if(NOT DEFINED HDF5_ROOT AND DEFINED ENV{HDF5_ROOT})
    set(HDF5_ROOT $ENV{HDF5_ROOT})
endif()
if(NOT DEFINED HDF5_DIR AND DEFINED ENV{HDF5_DIR})
    set(HDF5_DIR $ENV{HDF5_DIR})
endif()

if(HDF5_ROOT)
    list(APPEND _hdf5_candidate_prefixes ${HDF5_ROOT})
endif()
if(HDF5_DIR)
    list(APPEND _hdf5_candidate_prefixes ${HDF5_DIR})
    get_filename_component(_hdf5_dir_parent ${HDF5_DIR} DIRECTORY)
    if(_hdf5_dir_parent)
        list(APPEND _hdf5_candidate_prefixes ${_hdf5_dir_parent})
    endif()
endif()
if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(_prefix)
            list(APPEND _hdf5_candidate_prefixes ${_prefix})
        endif()
    endforeach()
endif()
list(REMOVE_DUPLICATES _hdf5_candidate_prefixes)

if(_hdf5_candidate_prefixes)
    set(_hdf5_candidate_includes "")
    set(_hdf5_candidate_libs "")
    foreach(_prefix IN LISTS _hdf5_candidate_prefixes)
        list(APPEND _hdf5_candidate_includes
            ${_prefix}/include
            ${_prefix}/Include
            ${_prefix}/include/hdf5
            ${_prefix}/include/hdf5/serial
        )
        list(APPEND _hdf5_candidate_libs
            ${_prefix}/lib
            ${_prefix}/Lib
            ${_prefix}/lib/release
            ${_prefix}/lib/shared
            ${_prefix}/bin
            ${_prefix}/Bin
        )
    endforeach()
    list(REMOVE_DUPLICATES _hdf5_candidate_includes)
    list(REMOVE_DUPLICATES _hdf5_candidate_libs)

    find_path(HDF5_INCLUDE_DIRS
        NAMES hdf5.h
        PATHS ${_hdf5_candidate_includes}
        NO_DEFAULT_PATH
        NO_CACHE
    )

    find_library(HDF5_C_LIBRARY
        NAMES hdf5 libhdf5 hdf5dll libhdf5dll
        PATHS ${_hdf5_candidate_libs}
        NO_DEFAULT_PATH
        NO_CACHE
    )

    find_library(HDF5_HL_LIBRARY
        NAMES hdf5_hl libhdf5_hl hdf5_hldll libhdf5_hldll
        PATHS ${_hdf5_candidate_libs}
        NO_DEFAULT_PATH
        NO_CACHE
    )

    if(HDF5_INCLUDE_DIRS AND HDF5_C_LIBRARY AND HDF5_HL_LIBRARY)
        set(_hdf5_found_manual ON)
        set(HDF5_FOUND TRUE)
        set(HDF5_LIBRARIES ${HDF5_C_LIBRARY} ${HDF5_HL_LIBRARY})
        set(HDF5_LIBRARY ${HDF5_C_LIBRARY})
        set(HDF5_INCLUDE_DIR ${HDF5_INCLUDE_DIRS})
        set(HDF5_HL_LIBRARIES ${HDF5_HL_LIBRARY})
        if(NOT HDF5_VERSION)
            set(HDF5_VERSION "manual")
        endif()
        message(STATUS "HDF5 located via manual search: ${HDF5_INCLUDE_DIRS}")
    else()
        message(STATUS "Manual HDF5 lookup failed with candidates:")
        message(STATUS "  Includes: ${_hdf5_candidate_includes}")
        message(STATUS "  Libs: ${_hdf5_candidate_libs}")
    endif()
endif()

if(NOT _hdf5_found_manual)
    find_package(HDF5 COMPONENTS C HL REQUIRED)
endif()

# Ensure high-level (H5LT) libs are linked
if(DEFINED HDF5_HL_LIBRARIES)
    set(_hdf5_hl_libs ${HDF5_HL_LIBRARIES})
elseif(DEFINED HDF5_C_HL_LIBRARY)
    set(_hdf5_hl_libs ${HDF5_C_HL_LIBRARY})
endif()

if(_hdf5_hl_libs)
    list(APPEND HDF5_LIBRARIES ${_hdf5_hl_libs})
endif()

message(STATUS "✓ Found HDF5 ${HDF5_VERSION}")
message(STATUS "  - Include dirs: ${HDF5_INCLUDE_DIRS}")
message(STATUS "  - Libraries: ${HDF5_LIBRARIES}")
if(_hdf5_hl_libs)
    message(STATUS "  - High-level libs: ${_hdf5_hl_libs}")
endif()

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