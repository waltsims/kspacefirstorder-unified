cmake_minimum_required(VERSION 3.22)
project(kspaceFirstOrder-OMP LANGUAGES CXX)

# ---------------- Options ----------------
option(USE_MKL "Use Intel oneMKL instead of FFTW (Linux easiest). On Windows CI we use FFTW." OFF)
set(ARCH "native" CACHE STRING "CPU arch: native|avx|avx2|avx512")
option(ENABLE_OPENMP "Enable OpenMP" ON)
option(FORCE_MSVC_LLVM_OPENMP "Use /openmp:llvm with MSVC 2022+" OFF)

# Git hash definition (fallback if not a git repo)
set(GIT_HASH "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
endif()
message(STATUS "__KWAVE_GIT_HASH__=${GIT_HASH}")

# ---------------- Compiler setup ----------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/permissive- /Zc:__cplusplus)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# SIMD flags
if(NOT MSVC)
  if(ARCH STREQUAL "avx")
    add_compile_options(-m64 -mavx)
  elseif(ARCH STREQUAL "avx2")
    add_compile_options(-m64 -mavx2)
  elseif(ARCH STREQUAL "avx512")
    add_compile_options(-m64 -mavx512f)
  else()
    add_compile_options(-m64 -march=native -mtune=native)
  endif()
else()
  if(ARCH STREQUAL "avx2")
    add_compile_options(/arch:AVX2)
  elseif(ARCH STREQUAL "avx512")
    add_compile_options(/arch:AVX512)
  endif()
endif()

# Optimize (match prior configs)
if(MSVC)
  add_compile_options(
    "$<$<CONFIG:Release>:/O2>"
    "$<$<CONFIG:Release>:/Ot>"
    "$<$<CONFIG:Release>:/GL>"
  )
else()
  add_compile_options(
    "$<$<CONFIG:Release>:-O3>"
    "$<$<CONFIG:Release>:-ffast-math>"
    "$<$<CONFIG:Release>:-fassociative-math>"
  )
endif()

# ---------------- OpenMP ----------------
if(ENABLE_OPENMP)
  if(MSVC)
    if(FORCE_MSVC_LLVM_OPENMP)
      add_compile_options(/openmp:llvm)
    else()
      add_compile_options(/openmp:experimental)
    endif()
  else()
    find_package(OpenMP REQUIRED)
    message(STATUS "OpenMP found: ${OpenMP_CXX_FLAGS}")
    add_compile_options(${OpenMP_CXX_FLAGS})
    if(OpenMP_CXX_LIB_NAMES)
      link_libraries(OpenMP::OpenMP_CXX)
    endif()
  endif()
endif()

# ---------------- Dependencies ----------------
set(HDF5_REQUIRED_COMPONENTS C HL)
find_package(HDF5 CONFIG QUIET)
if(NOT HDF5_FOUND)
  find_package(HDF5 REQUIRED COMPONENTS ${HDF5_REQUIRED_COMPONENTS})
endif()
message(STATUS "HDF5 found: ${HDF5_DIR} (targets or vars available)")

if(USE_MKL)
  find_package(MKL CONFIG REQUIRED)
  set(FFT_LIBS MKL::MKL)
  add_definitions(-DUSE_MKL_BACKEND)
else()
  find_package(FFTW3 CONFIG REQUIRED)
  if(NOT TARGET FFTW3::fftw3f)
    # vcpkg builds FFTW precision variants as separate packages, so pick up FFTW3f when needed.
    find_package(FFTW3f CONFIG QUIET)
  endif()

  set(_fftw_single_target "")
  foreach(candidate FFTW3::fftw3f FFTW3f::fftw3f fftw3f::fftw3f)
    if(TARGET ${candidate})
      set(_fftw_single_target ${candidate})
      break()
    endif()
  endforeach()

  set(_fftw_single_omp_target "")
  foreach(candidate FFTW3::fftw3f_omp FFTW3f::fftw3f_omp fftw3f::fftw3f_omp)
    if(TARGET ${candidate})
      set(_fftw_single_omp_target ${candidate})
      break()
    endif()
  endforeach()

  if(_fftw_single_target)
    if(_fftw_single_omp_target)
      set(FFT_LIBS ${_fftw_single_omp_target} ${_fftw_single_target})
    else()
      set(FFT_LIBS ${_fftw_single_target})
    endif()
  else()
    message(FATAL_ERROR
      "FFTW single-precision target not found. Install FFTW with float support (e.g. FFTW3f) "
      "or configure the build with USE_MKL=ON.")
  endif()
endif()

# ---------------- Sources ----------------
set(SRC
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
  MatrixClasses/FftwComplexMatrix.cpp
  MatrixClasses/FftwRealMatrix.cpp
  MatrixClasses/IndexMatrix.cpp
  MatrixClasses/RealMatrix.cpp
  OutputStreams/BaseOutputStream.cpp
  OutputStreams/CuboidOutputStream.cpp
  OutputStreams/IndexOutputStream.cpp
  OutputStreams/WholeDomainOutputStream.cpp
  Parameters/CommandLineParameters.cpp
  Parameters/Parameters.cpp
)

set(HDR
  Containers/MatrixContainer.h
  Containers/MatrixRecord.h
  Containers/OutputStreamContainer.h
  GetoptWin64/Getopt.h
  Hdf5/Hdf5File.h
  Hdf5/Hdf5FileHeader.h
  KSpaceSolver/KSpaceFirstOrderSolver.h
  Logger/ErrorMessages.h
  Logger/ErrorMessagesLinux.h
  Logger/ErrorMessagesWindows.h
  Logger/Logger.h
  Logger/OutputMessages.h
  Logger/OutputMessagesLinux.h
  Logger/OutputMessagesWindows.h
  MatrixClasses/BaseFloatMatrix.h
  MatrixClasses/BaseIndexMatrix.h
  MatrixClasses/BaseMatrix.h
  MatrixClasses/ComplexMatrix.h
  MatrixClasses/FftwComplexMatrix.h
  MatrixClasses/FftwRealMatrix.h
  MatrixClasses/IndexMatrix.h
  MatrixClasses/RealMatrix.h
  OutputStreams/BaseOutputStream.h
  OutputStreams/CuboidOutputStream.h
  OutputStreams/IndexOutputStream.h
  OutputStreams/WholeDomainOutputStream.h
  Parameters/CommandLineParameters.h
  Parameters/Parameters.h
  Utils/DimensionSizes.h
  Utils/TimeMeasure.h
)

add_executable(kspaceFirstOrder-OMP ${SRC} ${HDR})

target_compile_definitions(kspaceFirstOrder-OMP PRIVATE __KWAVE_GIT_HASH__="${GIT_HASH}")

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_include_directories(kspaceFirstOrder-OMP PRIVATE .)

if(TARGET hdf5::hdf5)
  target_link_libraries(kspaceFirstOrder-OMP PRIVATE hdf5::hdf5 hdf5::hdf5_hl)
elseif(HDF5_FOUND)
  target_include_directories(kspaceFirstOrder-OMP PRIVATE ${HDF5_INCLUDE_DIRS})
  target_link_libraries(kspaceFirstOrder-OMP PRIVATE ${HDF5_LIBRARIES})
  if(DEFINED HDF5_HL_LIBRARIES)
    target_link_libraries(kspaceFirstOrder-OMP PRIVATE ${HDF5_HL_LIBRARIES})
  endif()
endif()

target_link_libraries(kspaceFirstOrder-OMP PRIVATE ${FFT_LIBS})

if(ENABLE_OPENMP AND OpenMP_CXX_FOUND)
  target_link_libraries(kspaceFirstOrder-OMP PRIVATE OpenMP::OpenMP_CXX)
endif()

if(WIN32 AND MSVC)
  target_link_libraries(kspaceFirstOrder-OMP PRIVATE
    kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid odbc32 odbccp32
  )
endif()

if(MSVC)
  target_link_options(kspaceFirstOrder-OMP PRIVATE
    $<$<CONFIG:Release>:/INCREMENTAL:NO>
  )
else()
  target_link_options(kspaceFirstOrder-OMP PRIVATE
    $<$<CONFIG:Release>:-s>
  )
endif()
