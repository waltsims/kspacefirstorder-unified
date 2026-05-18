cmake_minimum_required(VERSION 3.16)

project(kspaceFirstOrderOMP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies ----------------------------------------------------------------
# Prefer static linking for FFTW and the C++ runtime so the released binary
# runs on systems without matching FFTW installed (the major v1.4.0
# regression).  HDF5 remains dynamic because distros ship a stable libhdf5
# ABI (libhdf5_serial.so.103 / hdf5_hl.so.100 on Ubuntu 22.04+) and pulling
# in static HDF5 drags transitive szip/zlib symbols that require explicit
# resolution.  glibc itself cannot be safely static-linked — the build
# runner's glibc version sets the floor.
find_package(HDF5 REQUIRED COMPONENTS C HL)
find_package(ZLIB REQUIRED)
find_package(SZIP QUIET)

find_path(KSPACE_FFTW_INCLUDE_DIR fftw3.h)
# Prefer the .a archive over .so so the binary doesn't depend on system FFTW.
find_library(KSPACE_FFTW_LIB NAMES libfftw3f.a fftw3f)
find_library(KSPACE_FFTW_OMP_LIB NAMES libfftw3f_omp.a fftw3f_omp)

if(NOT KSPACE_FFTW_INCLUDE_DIR OR NOT KSPACE_FFTW_LIB OR NOT KSPACE_FFTW_OMP_LIB)
  message(FATAL_ERROR "FFTW single-precision libraries (fftw3f, fftw3f_omp) not found. Add their location to CMAKE_PREFIX_PATH.")
endif()

find_package(OpenMP REQUIRED COMPONENTS CXX)

# Sources ---------------------------------------------------------------------
add_executable(kspaceFirstOrder-OMP
  main.cpp
  Containers/MatrixContainer.cpp
  Containers/OutputStreamContainer.cpp
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

target_include_directories(kspaceFirstOrder-OMP
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${KSPACE_FFTW_INCLUDE_DIR}
    ${HDF5_INCLUDE_DIRS}
)

target_link_libraries(kspaceFirstOrder-OMP PRIVATE OpenMP::OpenMP_CXX)

set(_hdf5_libs ${HDF5_LIBRARIES})
if(DEFINED HDF5_HL_LIBRARIES)
  list(APPEND _hdf5_libs ${HDF5_HL_LIBRARIES})
endif()

target_link_libraries(kspaceFirstOrder-OMP
  PRIVATE
    ${KSPACE_FFTW_LIB}
    ${KSPACE_FFTW_OMP_LIB}
    ${_hdf5_libs}
    ZLIB::ZLIB
)

if(SZIP_FOUND)
  target_link_libraries(kspaceFirstOrder-OMP PRIVATE SZIP::SZIP)
endif()

if(CMAKE_DL_LIBS)
  target_link_libraries(kspaceFirstOrder-OMP PRIVATE ${CMAKE_DL_LIBS})
endif()

target_link_libraries(kspaceFirstOrder-OMP PRIVATE m)
target_compile_options(kspaceFirstOrder-OMP PRIVATE -O3 -Wall)

# Static C++ runtime so the binary runs on distros with older libstdc++ /
# libgcc_s than the build runner; pairs with the static FFTW/HDF5 link above
# to make the executable self-contained.
target_link_options(kspaceFirstOrder-OMP PRIVATE
  -static-libgcc
  -static-libstdc++
)

set_target_properties(kspaceFirstOrder-OMP PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
