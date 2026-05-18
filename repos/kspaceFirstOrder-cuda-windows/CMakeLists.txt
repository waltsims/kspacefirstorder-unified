cmake_minimum_required(VERSION 3.24)

if(POLICY CMP0104)
  cmake_policy(SET CMP0104 NEW) # CUDA architecture handling
endif()
if(POLICY CMP0146)
  cmake_policy(SET CMP0146 NEW) # CUDA Clang host compilation
endif()

project(kspaceFirstOrderCUDA LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------------------------------------------------
# Options mirroring the legacy Makefile and Visual Studio switches
# -------------------------------------------------------------------------------------------------
set(KSPACE_CPU_ARCH "native" CACHE STRING "Target CPU ISA tuning (native, avx, avx2, avx512).")
set_property(CACHE KSPACE_CPU_ARCH PROPERTY STRINGS native avx avx2 avx512)

option(KSPACE_ENABLE_OPENMP "Enable OpenMP parallelism in host code." ON)
option(KSPACE_ENABLE_FAST_MATH "Enable aggressive fast-math flags on the host compiler." ON)

set(KSPACE_CUDA_ARCH_LIST "" CACHE STRING "Override CUDA architectures (semicolon, space, or comma separated).")

if(KSPACE_CUDA_ARCH_LIST)
  set(CMAKE_CUDA_ARCHITECTURES "${KSPACE_CUDA_ARCH_LIST}")
elseif(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  # Default to modern architectures supported by current CUDA toolkits.
  # sm_100/sm_120 (Blackwell) require CUDA 12.8+; SetupCUDA.cmake drops
  # them automatically when an older toolchain is detected.
  # Override via KSPACE_CUDA_ARCH_LIST if needed.
  set(CMAKE_CUDA_ARCHITECTURES "75;80;86;87;89;90;90a;100;120")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SetupCUDA.cmake)

set(CMAKE_CUDA_STANDARD 11)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)

if(MSVC AND KSPACE_ENABLE_OPENMP)
  message(WARNING "Disabling OpenMP: MSVC requires signed loop indices and /openmp:experimental, which the current codebase does not satisfy.")
  set(KSPACE_ENABLE_OPENMP OFF CACHE BOOL "Enable OpenMP parallelism in host code." FORCE)
endif()

# -------------------------------------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------------------------------------
find_package(CUDAToolkit REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C HL)
find_package(ZLIB REQUIRED)
find_package(SZIP QUIET)

if(KSPACE_ENABLE_OPENMP)
  find_package(OpenMP)
  if(NOT OpenMP_CXX_FOUND)
    message(WARNING "OpenMP requested but not found; disabling.")
    set(KSPACE_ENABLE_OPENMP OFF CACHE BOOL "Enable OpenMP parallelism in host code." FORCE)
  endif()
endif()

# -------------------------------------------------------------------------------------------------
# Source lists
# -------------------------------------------------------------------------------------------------
set(KSPACE_CXX_SOURCES
  main.cpp
  Containers/MatrixContainer.cpp
  Containers/OutputStreamContainer.cpp
  GetoptWin64/Getopt.cpp
  Hdf5/Hdf5File.cpp
  Hdf5/Hdf5FileHeader.cpp
  KSpaceSolver/KSpaceFirstOrderSolver.cpp
  Logger/Logger.cpp
  MatrixClasses/BaseFloatMatrix.cpp
  MatrixClasses/BaseIndexMatrix.cpp
  MatrixClasses/ComplexMatrix.cpp
  MatrixClasses/CufftComplexMatrix.cpp
  MatrixClasses/IndexMatrix.cpp
  MatrixClasses/RealMatrix.cpp
  OutputStreams/BaseOutputStream.cpp
  OutputStreams/CuboidOutputStream.cpp
  OutputStreams/IndexOutputStream.cpp
  OutputStreams/WholeDomainOutputStream.cpp
  Parameters/CommandLineParameters.cpp
  Parameters/CudaParameters.cpp
  Parameters/Parameters.cpp
)

set(KSPACE_CUDA_SOURCES
  Containers/CudaMatrixContainer.cu
  KSpaceSolver/SolverCudaKernels.cu
  MatrixClasses/TransposeCudaKernels.cu
  OutputStreams/OutputStreamsCudaKernels.cu
  Parameters/CudaDeviceConstants.cu
)

add_executable(kspaceFirstOrder-CUDA
  ${KSPACE_CXX_SOURCES}
  ${KSPACE_CUDA_SOURCES}
)

target_include_directories(kspaceFirstOrder-CUDA
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_property(TARGET kspaceFirstOrder-CUDA PROPERTY CUDA_SEPARABLE_COMPILATION ON)
set_property(TARGET kspaceFirstOrder-CUDA PROPERTY CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}")

# -------------------------------------------------------------------------------------------------
# Host and device compiler flags
# -------------------------------------------------------------------------------------------------
set(_cpu_arch_value "${KSPACE_CPU_ARCH}")
string(TOLOWER "${_cpu_arch_value}" _cpu_arch_value)

set(_cpu_arch_flags "")
if(MSVC)
  if(_cpu_arch_value STREQUAL "avx")
    list(APPEND _cpu_arch_flags "/arch:AVX")
  elseif(_cpu_arch_value STREQUAL "avx2")
    list(APPEND _cpu_arch_flags "/arch:AVX2")
  elseif(_cpu_arch_value STREQUAL "avx512")
    message(FATAL_ERROR "KSPACE_CPU_ARCH=avx512 is not supported with MSVC toolchains.")
  elseif(NOT _cpu_arch_value STREQUAL "native")
    message(FATAL_ERROR "Unsupported value for KSPACE_CPU_ARCH: ${KSPACE_CPU_ARCH}")
  endif()
else()
  if(_cpu_arch_value STREQUAL "native")
    list(APPEND _cpu_arch_flags "-march=native" "-mtune=native")
  elseif(_cpu_arch_value STREQUAL "avx")
    list(APPEND _cpu_arch_flags "-m64" "-mavx")
  elseif(_cpu_arch_value STREQUAL "avx2")
    list(APPEND _cpu_arch_flags "-m64" "-mavx2")
  elseif(_cpu_arch_value STREQUAL "avx512")
    list(APPEND _cpu_arch_flags "-m64" "-mavx512f")
  else()
    message(FATAL_ERROR "Unsupported value for KSPACE_CPU_ARCH: ${KSPACE_CPU_ARCH}")
  endif()
endif()

set(_host_opt_flags "")
if(MSVC)
  list(APPEND _host_opt_flags "/O2" "/MP")
  if(KSPACE_ENABLE_FAST_MATH)
    list(APPEND _host_opt_flags "/fp:fast")
  endif()
else()
  list(APPEND _host_opt_flags "-O3")
  if(KSPACE_ENABLE_FAST_MATH)
    list(APPEND _host_opt_flags "-ffast-math" "-fassociative-math")
  endif()
endif()

if(KSPACE_ENABLE_OPENMP AND OpenMP_CXX_FOUND)
  set(_openmp_flags "${OpenMP_CXX_FLAGS}")
  if(_openmp_flags)
    separate_arguments(_openmp_flag_list NATIVE_COMMAND "${_openmp_flags}")
    list(APPEND _host_opt_flags ${_openmp_flag_list})
  endif()
  target_link_libraries(kspaceFirstOrder-CUDA PRIVATE OpenMP::OpenMP_CXX)
endif()

target_compile_options(kspaceFirstOrder-CUDA
  PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${_cpu_arch_flags}>
    $<$<COMPILE_LANGUAGE:CXX>:${_host_opt_flags}>
)

set(_cuda_compile_flags "-O3" "--restrict" "--device-c")
foreach(flag IN LISTS _cpu_arch_flags _host_opt_flags)
  list(APPEND _cuda_compile_flags "-Xcompiler=${flag}")
endforeach()

target_compile_options(kspaceFirstOrder-CUDA
  PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:${_cuda_compile_flags}>
)

# -------------------------------------------------------------------------------------------------
# Defines and link libraries
# -------------------------------------------------------------------------------------------------
target_compile_definitions(kspaceFirstOrder-CUDA
  PRIVATE
    $<$<AND:$<BOOL:${WIN32}>,$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>>:WIN64>
    _CONSOLE
)

target_link_libraries(kspaceFirstOrder-CUDA
  PRIVATE
    CUDA::cudart
    CUDA::cufft
    HDF5::HDF5
    hdf5::hdf5_hl
    ZLIB::ZLIB
)

if(SZIP_FOUND)
  target_link_libraries(kspaceFirstOrder-CUDA PRIVATE SZIP::SZIP)
endif()

# TODO: Revisit Windows-specific runtime library selection (static vs dynamic CUDA).
# TODO: Evaluate vcpkg integration to surface dependencies consistently across platforms.
