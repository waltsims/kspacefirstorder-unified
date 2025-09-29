######################################################################
# HDF5
######################################################################

# Prefer serial HDF5 unless parallel is explicitly requested externally
set(HDF5_PREFER_PARALLEL OFF CACHE BOOL "Prefer parallel HDF5 (MPI)")

# 1) Prefer config packages (modern, provide imported targets)
# 2) Fallback to module
# 3) Last resort: manual search using env/CMAKE_PREFIX_PATH hints

set(_hdf5_found FALSE)
set(_hdf5_found_manual FALSE)

# Try config mode first
find_package(HDF5 CONFIG QUIET COMPONENTS C HL)
if(HDF5_FOUND)
    set(_hdf5_found TRUE)
else()
    # Try classic Find module next
    find_package(HDF5 MODULE QUIET COMPONENTS C HL)
    if(HDF5_FOUND)
        set(_hdf5_found TRUE)
    endif()
endif()

# Manual fallback for non-standard layouts when both fail
if(NOT _hdf5_found)
    set(_hdf5_candidate_prefixes "")
    if(NOT DEFINED HDF5_ROOT AND DEFINED ENV{HDF5_ROOT})
        set(HDF5_ROOT $ENV{HDF5_ROOT})
    endif()
    if(NOT DEFINED HDF5_DIR AND DEFINED ENV{HDF5_DIR})
        set(HDF5_DIR $ENV{HDF5_DIR})
    endif()
    if(HDF5_ROOT)
        file(TO_CMAKE_PATH "${HDF5_ROOT}" _hdf5_root_norm)
        list(APPEND _hdf5_candidate_prefixes ${_hdf5_root_norm})
    endif()
    if(HDF5_DIR)
        file(TO_CMAKE_PATH "${HDF5_DIR}" _hdf5_dir_norm)
        list(APPEND _hdf5_candidate_prefixes ${_hdf5_dir_norm})
        if(EXISTS "${_hdf5_dir_norm}/..")
            get_filename_component(_hdf5_dir_parent ${_hdf5_dir_norm} DIRECTORY)
            if(_hdf5_dir_parent)
                list(APPEND _hdf5_candidate_prefixes ${_hdf5_dir_parent})
            endif()
        endif()
    endif()
    if(CMAKE_PREFIX_PATH)
        foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
            if(_prefix)
                file(TO_CMAKE_PATH "${_prefix}" _prefix_norm)
                list(APPEND _hdf5_candidate_prefixes ${_prefix_norm})
            endif()
        endforeach()
    endif()
    list(REMOVE_DUPLICATES _hdf5_candidate_prefixes)

    if(_hdf5_candidate_prefixes)
        find_path(HDF5_INCLUDE_DIRS
            NAMES hdf5.h H5public.h
            PATHS ${_hdf5_candidate_prefixes}
            PATH_SUFFIXES
                include
                Include
                include/release
                include/Release
                include/hdf5
                include/hdf5/serial
            NO_DEFAULT_PATH
            NO_CACHE
        )

        find_library(HDF5_C_LIBRARY
            NAMES hdf5 libhdf5 hdf5dll libhdf5dll hdf5.lib
            PATHS ${_hdf5_candidate_prefixes}
            PATH_SUFFIXES
                lib
                Lib
                lib/release
                lib/Release
                lib/shared
                bin
                Bin
            NO_DEFAULT_PATH
            NO_CACHE
        )

        find_library(HDF5_HL_LIBRARY
            NAMES hdf5_hl libhdf5_hl hdf5_hldll libhdf5_hldll hdf5_hl.lib
            PATHS ${_hdf5_candidate_prefixes}
            PATH_SUFFIXES
                lib
                Lib
                lib/release
                lib/Release
                lib/shared
                bin
                Bin
            NO_DEFAULT_PATH
            NO_CACHE
        )

        if(HDF5_INCLUDE_DIRS AND HDF5_C_LIBRARY AND HDF5_HL_LIBRARY)
            set(_hdf5_found_manual TRUE)
            set(HDF5_FOUND TRUE)
            set(HDF5_LIBRARIES ${HDF5_C_LIBRARY} ${HDF5_HL_LIBRARY})
            set(HDF5_LIBRARY ${HDF5_C_LIBRARY})
            set(HDF5_HL_LIBRARIES ${HDF5_HL_LIBRARY})
            if(NOT HDF5_VERSION)
                set(HDF5_VERSION "manual")
            endif()
            message(STATUS "HDF5 located via manual search: ${HDF5_INCLUDE_DIRS}")
        else()
            message(STATUS "Manual HDF5 lookup failed with prefixes: ${_hdf5_candidate_prefixes}")
        endif()
    endif()
endif()

if(NOT HDF5_FOUND)
    message(FATAL_ERROR "HDF5 not found. Set HDF5_DIR or HDF5_ROOT, or install HDF5 (see DEPENDENCIES.md)")
endif()

# Populate include dirs if still empty (e.g., some config packages don't set variables)
if(NOT HDF5_INCLUDE_DIRS)
    find_path(HDF5_INCLUDE_DIRS
        NAMES hdf5.h H5public.h
        PATH_SUFFIXES
            include
            include/hdf5
            include/hdf5/serial
    )
endif()

# Populate libraries if still empty and no imported targets
if(NOT _hdf5_core_target AND NOT HDF5_LIBRARIES)
    find_library(HDF5_C_LIBRARY
        NAMES hdf5 libhdf5
    )
    find_library(HDF5_HL_LIBRARY
        NAMES hdf5_hl libhdf5_hl
    )
    if(HDF5_C_LIBRARY)
        set(HDF5_LIBRARIES ${HDF5_C_LIBRARY})
        if(HDF5_HL_LIBRARY)
            list(APPEND HDF5_LIBRARIES ${HDF5_HL_LIBRARY})
        endif()
    endif()
endif()

# Build a thin interface target to propagate includes and link libs consistently
add_library(kspace_hdf5 INTERFACE)

# Prefer imported targets when available (config mode)
set(_hdf5_core_target "")
set(_hdf5_hl_target "")
# Try common target name variants from different package providers
foreach(_cand IN ITEMS 
    hdf5::hdf5 hdf5::hdf5-shared hdf5::hdf5-static 
    HDF5::HDF5 HDF5::hdf5 HDF5::hdf5-shared HDF5::hdf5-static)
    if(TARGET ${_cand})
        set(_hdf5_core_target ${_cand})
        break()
    endif()
endforeach()
foreach(_cand IN ITEMS 
    hdf5::hdf5_hl hdf5::hdf5_hl-shared hdf5::hdf5_hl-static 
    HDF5::HDF5_hl HDF5::hdf5_hl HDF5::hdf5_hl-shared HDF5::hdf5_hl-static)
    if(TARGET ${_cand})
        set(_hdf5_hl_target ${_cand})
        break()
    endif()
endforeach()

if(_hdf5_core_target)
    target_link_libraries(kspace_hdf5 INTERFACE ${_hdf5_core_target})
    # Explicitly propagate include dirs from the imported target
    target_include_directories(kspace_hdf5 INTERFACE
        $<TARGET_PROPERTY:${_hdf5_core_target},INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()
if(_hdf5_hl_target)
    target_link_libraries(kspace_hdf5 INTERFACE ${_hdf5_hl_target})
endif()

# If no imported targets, fall back to variables from Find module/manual search
if(NOT _hdf5_core_target)
    if(DEFINED HDF5_LIBRARIES)
        target_link_libraries(kspace_hdf5 INTERFACE ${HDF5_LIBRARIES})
    endif()
endif()

# Always add include directories if discovered
if(DEFINED HDF5_INCLUDE_DIRS)
    target_include_directories(kspace_hdf5 INTERFACE ${HDF5_INCLUDE_DIRS})
endif()

message(STATUS "✓ Found HDF5 ${HDF5_VERSION}")
# If variables are empty in config mode, try to populate for logging
if(NOT HDF5_INCLUDE_DIRS AND _hdf5_core_target)
    get_target_property(_h5_includes ${_hdf5_core_target} INTERFACE_INCLUDE_DIRECTORIES)
    if(_h5_includes)
        set(HDF5_INCLUDE_DIRS ${_h5_includes})
    endif()
endif()
if(NOT HDF5_LIBRARIES AND _hdf5_core_target)
    set(HDF5_LIBRARIES ${_hdf5_core_target})
    if(_hdf5_hl_target)
        list(APPEND HDF5_LIBRARIES ${_hdf5_hl_target})
    endif()
endif()
message(STATUS "  - Include dirs: ${HDF5_INCLUDE_DIRS}")
message(STATUS "  - Libraries: ${HDF5_LIBRARIES}")

# Find FFTW3 (for OpenMP backend)
if(USE_OPENMP)
    # Prefer config packages (vcpkg provides imported targets): FFTW3::fftw3f and FFTW3::fftw3f_threads
    set(FFTW_FOUND FALSE)
    set(_fftwf_target "")
    set(_fftwf_threads_target "")

    find_package(FFTW3 CONFIG QUIET)

    foreach(_cand IN ITEMS FFTW3::fftw3f fftw3f FFTW3::FFTW3F)
        if(TARGET ${_cand})
            set(_fftwf_target ${_cand})
            break()
        endif()
    endforeach()
    foreach(_cand IN ITEMS FFTW3::fftw3f_threads fftw3f_threads FFTW3::FFTW3F_THREADS)
        if(TARGET ${_cand})
            set(_fftwf_threads_target ${_cand})
            break()
        endif()
    endforeach()

    if(_fftwf_target)
        set(FFTW_FOUND TRUE)
        set(FFTW_LIBRARIES ${_fftwf_target})
        if(_fftwf_threads_target)
            list(APPEND FFTW_LIBRARIES ${_fftwf_threads_target})
        endif()
        # Propagate includes from imported targets
        get_target_property(_fftw_inc ${_fftwf_target} INTERFACE_INCLUDE_DIRECTORIES)
        if(_fftw_inc)
            set(FFTW_INCLUDE_DIRS ${_fftw_inc})
        endif()
    endif()

    # Fallback: manual search for single-precision FFTW
    if(NOT FFTW_FOUND)
        # Define search paths
        set(FFTW_SEARCH_PATHS
            /usr/lib
            /usr/local/lib
            /opt/local/lib
            /opt/homebrew/lib
            $ENV{FFTW_ROOT}/lib
            $ENV{LOCALAPPDATA}/fftw
            ${CMAKE_SOURCE_DIR}/third-party/fftw
        )
        set(FFTW_INCLUDE_SEARCH_PATHS
            /usr/include
            /usr/local/include
            /opt/local/include
            /opt/homebrew/include
            $ENV{FFTW_ROOT}/include
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
    endif()

    if(NOT FFTW_FOUND)
        message(FATAL_ERROR "FFTW3 (single precision) not found. Please install via vcpkg or system packages (see docs).")
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
