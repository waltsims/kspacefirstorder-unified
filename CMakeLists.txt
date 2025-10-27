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

# ---------------- Compiler guard ----------------
if(NOT MSVC)
  message(FATAL_ERROR "This build currently supports MSVC only.")
endif()

# ---------------- OpenMP ----------------
if(ENABLE_OPENMP)
  if(FORCE_MSVC_LLVM_OPENMP)
    add_compile_options(/openmp:llvm)
  else()
    add_compile_options(/openmp:experimental)
  endif()
endif()

# ---------------- Dependencies ----------------
find_package(HDF5 CONFIG REQUIRED COMPONENTS C HL)
set(_hdf5_candidates
  HDF5::HDF5
  hdf5::hdf5-shared
  hdf5::hdf5)
set(_hdf5_hl_candidates
  HDF5::HDF5_HL
  hdf5::hdf5_hl-shared
  hdf5::hdf5_hl)
set(HDF5_C_TARGET "")
foreach(candidate ${_hdf5_candidates})
  if(TARGET ${candidate})
    set(HDF5_C_TARGET ${candidate})
    break()
  endif()
endforeach()
if(NOT HDF5_C_TARGET)
  message(FATAL_ERROR "Unable to locate HDF5 C target from expected candidates: ${_hdf5_candidates}")
endif()
set(HDF5_HL_TARGET "")
foreach(candidate ${_hdf5_hl_candidates})
  if(TARGET ${candidate})
    set(HDF5_HL_TARGET ${candidate})
    break()
  endif()
endforeach()
if(NOT HDF5_HL_TARGET)
  message(FATAL_ERROR "Unable to locate HDF5 HL target from expected candidates: ${_hdf5_hl_candidates}")
endif()

if(USE_MKL)
  find_package(MKL CONFIG REQUIRED)
  set(SOLVER_LIBS MKL::MKL)
  add_definitions(-DUSE_MKL_BACKEND)
  else()
    find_package(FFTW3 CONFIG REQUIRED)
    # vcpkg packages the single-precision libs as fftw3f, so try to load that config as well.
    find_package(fftw3f CONFIG QUIET)
    set(_fftw_float_candidates
      FFTW3::fftw3f
      FFTW3f::fftw3f
      fftw3::fftw3f
      fftw3f::fftw3f)
    set(_fftw_threads_candidates
      FFTW3::fftw3f_threads
      FFTW3f::fftw3f_threads
      fftw3::fftw3f_threads
      fftw3f::fftw3f_threads)

  set(FFTW_FLOAT_TARGET "")
  foreach(candidate ${_fftw_float_candidates})
    if(TARGET ${candidate})
      set(FFTW_FLOAT_TARGET ${candidate})
      break()
    endif()
  endforeach()
  if(NOT FFTW_FLOAT_TARGET)
    message(FATAL_ERROR "Unable to locate FFTW float target from expected candidates: ${_fftw_float_candidates}")
  endif()

  set(FFTW_FLOAT_THREADS_TARGET "")
  foreach(candidate ${_fftw_threads_candidates})
    if(TARGET ${candidate})
      set(FFTW_FLOAT_THREADS_TARGET ${candidate})
      break()
    endif()
  endforeach()
  if(NOT FFTW_FLOAT_THREADS_TARGET)
    message(STATUS "FFTW float threads target not found; assuming threads combined into ${FFTW_FLOAT_TARGET}")
    set(FFTW_FLOAT_THREADS_TARGET ${FFTW_FLOAT_TARGET})
  endif()

  if("${FFTW_FLOAT_THREADS_TARGET}" STREQUAL "${FFTW_FLOAT_TARGET}")
    set(SOLVER_LIBS ${FFTW_FLOAT_TARGET})
  else()
    set(SOLVER_LIBS ${FFTW_FLOAT_THREADS_TARGET} ${FFTW_FLOAT_TARGET})
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
  Utils/OmpHelpers.h
)

add_executable(kspaceFirstOrder-OMP ${SRC} ${HDR})

target_compile_definitions(kspaceFirstOrder-OMP PRIVATE __KWAVE_GIT_HASH__="${GIT_HASH}")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(kspaceFirstOrder-OMP
  PRIVATE
    ${HDF5_C_TARGET}
    ${HDF5_HL_TARGET}
    ${SOLVER_LIBS}
)

target_include_directories(kspaceFirstOrder-OMP PRIVATE .)

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
