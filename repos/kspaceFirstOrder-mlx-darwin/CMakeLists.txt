cmake_minimum_required(VERSION 3.28)

project(kspaceFirstOrder-MLX
        VERSION 1.3
        DESCRIPTION "k-Wave kspaceFirstOrder GPU solver ported from CUDA to MLX"
        LANGUAGES C CXX)

#--------------------------------------------------------------------------------------------------------------------#
# Options
#--------------------------------------------------------------------------------------------------------------------#
# Use an installed MLX (e.g. `brew install mlx`) by default. Set KWAVE_FETCH_MLX=ON to build MLX from source instead,
# which is the way to get the CUDA backend on Linux (add -DMLX_BUILD_CUDA=ON).
option(KWAVE_FETCH_MLX  "Download and build MLX from source"        OFF)
set(KWAVE_MLX_VERSION   "v0.32.4" CACHE STRING "MLX version used when KWAVE_FETCH_MLX=ON")
option(KWAVE_USE_OPENMP "Use OpenMP for host-side preprocessing"   ON)
# Self-contained build for redistribution (e.g. GitHub release assets): MLX is built from source as a static library,
# HDF5 (+ szip/libaec) and libomp are linked statically, so the binary only depends on macOS system libraries and
# frameworks. The MLX Metal kernels (mlx.metallib) are copied next to the executable and must be shipped with it.
option(KWAVE_BUNDLE_DEPS "Link MLX, HDF5 and OpenMP statically (redistributable binary + mlx.metallib)" OFF)
option(KWAVE_MLX_METAL_JIT "Build MLX with run-time compiled Metal kernels (small mlx.metallib)" ${KWAVE_BUNDLE_DEPS})
if (KWAVE_BUNDLE_DEPS)
  set(KWAVE_FETCH_MLX ON CACHE BOOL "" FORCE)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# MLX headers require C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#--------------------------------------------------------------------------------------------------------------------#
# Dependencies
#--------------------------------------------------------------------------------------------------------------------#
# MLX
if (KWAVE_FETCH_MLX)
  include(FetchContent)
  set(MLX_BUILD_TESTS            OFF CACHE BOOL "" FORCE)
  set(MLX_BUILD_EXAMPLES         OFF CACHE BOOL "" FORCE)
  set(MLX_BUILD_BENCHMARKS       OFF CACHE BOOL "" FORCE)
  set(MLX_BUILD_PYTHON_BINDINGS  OFF CACHE BOOL "" FORCE)
  set(MLX_BUILD_GGUF             OFF CACHE BOOL "" FORCE)
  set(MLX_BUILD_SAFETENSORS      OFF CACHE BOOL "" FORCE)
  # JIT-compile the Metal kernels at run time instead of shipping all of them precompiled: mlx.metallib shrinks
  # from ~150 MB to a few MB.
  set(MLX_METAL_JIT              ${KWAVE_MLX_METAL_JIT} CACHE BOOL "" FORCE)
  if (KWAVE_BUNDLE_DEPS)
    set(BUILD_SHARED_LIBS        OFF CACHE BOOL "" FORCE)
  endif()
  FetchContent_Declare(mlx
                       GIT_REPOSITORY https://github.com/ml-explore/mlx.git
                       GIT_TAG        ${KWAVE_MLX_VERSION}
                       EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(mlx)
else()
  find_package(MLX CONFIG REQUIRED)
endif()

# HDF5 (C + high level library)
if (APPLE)
  execute_process(COMMAND brew --prefix hdf5
                  OUTPUT_VARIABLE KWAVE_HDF5_PREFIX
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET)
  if (KWAVE_HDF5_PREFIX)
    list(APPEND CMAKE_PREFIX_PATH "${KWAVE_HDF5_PREFIX}")
  endif()
  execute_process(COMMAND brew --prefix libaec
                  OUTPUT_VARIABLE KWAVE_LIBAEC_PREFIX
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET)
endif()

if (KWAVE_BUNDLE_DEPS)
  # Static HDF5 and its compression filters (Homebrew ships the .a files). zlib comes from the system.
  find_path(KWAVE_HDF5_INCLUDE_DIR hdf5.h REQUIRED)
  find_library(KWAVE_HDF5_STATIC    NAMES libhdf5.a    REQUIRED)
  find_library(KWAVE_HDF5_HL_STATIC NAMES libhdf5_hl.a REQUIRED)
  find_library(KWAVE_SZ_STATIC      NAMES libsz.a      HINTS ${KWAVE_LIBAEC_PREFIX}/lib)
  find_library(KWAVE_AEC_STATIC     NAMES libaec.a     HINTS ${KWAVE_LIBAEC_PREFIX}/lib)
  find_package(ZLIB REQUIRED)
  set(KWAVE_HDF5_TARGETS ${KWAVE_HDF5_HL_STATIC} ${KWAVE_HDF5_STATIC})
  foreach(lib KWAVE_SZ_STATIC KWAVE_AEC_STATIC)
    if (${lib})
      list(APPEND KWAVE_HDF5_TARGETS ${${lib}})
    endif()
  endforeach()
  list(APPEND KWAVE_HDF5_TARGETS ZLIB::ZLIB ${CMAKE_DL_LIBS})
else()
  find_package(HDF5 REQUIRED COMPONENTS C HL)
  set(KWAVE_HDF5_TARGETS HDF5::HDF5 ${HDF5_HL_LIBRARIES} ${HDF5_C_HL_LIBRARIES})
endif()

# OpenMP (optional, only used for host-side preprocessing loops)
if (KWAVE_USE_OPENMP)
  if (APPLE)
    # Apple clang does not ship libomp; use Homebrew's if available.
    execute_process(COMMAND brew --prefix libomp
                    OUTPUT_VARIABLE KWAVE_LIBOMP_PREFIX
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if (KWAVE_LIBOMP_PREFIX)
      list(APPEND CMAKE_PREFIX_PATH "${KWAVE_LIBOMP_PREFIX}")
      set(OpenMP_ROOT "${KWAVE_LIBOMP_PREFIX}")
      if (KWAVE_BUNDLE_DEPS AND EXISTS "${KWAVE_LIBOMP_PREFIX}/lib/libomp.a")
        # Link the static libomp so the binary does not need Homebrew at run time.
        set(KWAVE_LIBOMP_STATIC "${KWAVE_LIBOMP_PREFIX}/lib/libomp.a")
      endif()
    endif()
  endif()
  find_package(OpenMP COMPONENTS CXX)
  if (NOT OpenMP_CXX_FOUND)
    message(STATUS "OpenMP not found - host-side preprocessing will run single-threaded")
  endif()
endif()

#--------------------------------------------------------------------------------------------------------------------#
# Sources
#--------------------------------------------------------------------------------------------------------------------#
set(KWAVE_SOURCES
    main.cpp
    Containers/MatrixContainer.cpp
    Containers/OutputStreamContainer.cpp
    Hdf5/Hdf5File.cpp
    Hdf5/Hdf5FileHeader.cpp
    KSpaceSolver/KSpaceFirstOrderSolver.cpp
    KSpaceSolver/SolverMlxKernels.cpp
    Logger/Logger.cpp
    MatrixClasses/BaseFloatMatrix.cpp
    MatrixClasses/BaseIndexMatrix.cpp
    MatrixClasses/ComplexMatrix.cpp
    MatrixClasses/FftComplexMatrix.cpp
    MatrixClasses/IndexMatrix.cpp
    MatrixClasses/RealMatrix.cpp
    OutputStreams/BaseOutputStream.cpp
    OutputStreams/CuboidOutputStream.cpp
    OutputStreams/IndexOutputStream.cpp
    OutputStreams/WholeDomainOutputStream.cpp
    Parameters/CommandLineParameters.cpp
    Parameters/MlxParameters.cpp
    Parameters/Parameters.cpp)

if (WIN32)
  list(APPEND KWAVE_SOURCES GetoptWin64/Getopt.cpp)
endif()

add_executable(kspaceFirstOrder-MLX ${KWAVE_SOURCES})

target_include_directories(kspaceFirstOrder-MLX PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kspaceFirstOrder-MLX PRIVATE mlx ${KWAVE_HDF5_TARGETS})
if (KWAVE_BUNDLE_DEPS)
  target_include_directories(kspaceFirstOrder-MLX PRIVATE ${KWAVE_HDF5_INCLUDE_DIR})
endif()

# With a static MLX, the Metal kernels are looked up next to the executable: copy mlx.metallib there (and install it
# there too, see the install rules at the end).
set(KWAVE_COLOCATED_METALLIB OFF)
if (KWAVE_FETCH_MLX AND APPLE AND NOT BUILD_SHARED_LIBS)
  set(KWAVE_COLOCATED_METALLIB ON)
  set(KWAVE_METALLIB "${mlx_BINARY_DIR}/mlx/backend/metal/kernels/mlx.metallib")
  add_dependencies(kspaceFirstOrder-MLX mlx-metallib)
  add_custom_command(TARGET kspaceFirstOrder-MLX POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                             "${KWAVE_METALLIB}"
                             "$<TARGET_FILE_DIR:kspaceFirstOrder-MLX>/mlx.metallib"
                     COMMENT "Copying mlx.metallib next to kspaceFirstOrder-MLX")
endif()

if (OpenMP_CXX_FOUND)
  if (KWAVE_LIBOMP_STATIC)
    separate_arguments(KWAVE_OPENMP_FLAGS NATIVE_COMMAND "${OpenMP_CXX_FLAGS}")
    target_compile_options(kspaceFirstOrder-MLX PRIVATE ${KWAVE_OPENMP_FLAGS})
    target_include_directories(kspaceFirstOrder-MLX PRIVATE ${OpenMP_CXX_INCLUDE_DIRS})
    target_link_libraries(kspaceFirstOrder-MLX PRIVATE ${KWAVE_LIBOMP_STATIC})
  else()
    target_link_libraries(kspaceFirstOrder-MLX PRIVATE OpenMP::OpenMP_CXX)
  endif()
endif()

# Embed the git hash of the build (printed with --version).
find_package(Git QUIET)
if (GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  OUTPUT_VARIABLE KWAVE_GIT_HASH
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET)
endif()
if (KWAVE_GIT_HASH)
  target_compile_definitions(kspaceFirstOrder-MLX PRIVATE __KWAVE_GIT_HASH__="${KWAVE_GIT_HASH}")
endif()

if (MSVC)
  target_compile_options(kspaceFirstOrder-MLX PRIVATE /W3)
else()
  target_compile_options(kspaceFirstOrder-MLX PRIVATE -Wall -Wno-unknown-pragmas)
endif()

install(TARGETS kspaceFirstOrder-MLX RUNTIME DESTINATION bin)
if (KWAVE_COLOCATED_METALLIB)
  # The installed executable needs its Metal kernel library in the same folder.
  install(FILES "${KWAVE_METALLIB}" DESTINATION bin)
endif()
