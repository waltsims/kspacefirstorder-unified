# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DWIN32)
    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()
else()
    set(PLATFORM_UNIX TRUE)
    add_definitions(-DUNIX)
endif()

# Common compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warning flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
        if(ENABLE_NATIVE_ARCH)
            add_compile_options(-march=native)
        endif()
    endif()
endif()

# CUDA specific settings
if(USE_CUDA)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 50 52 53 60 61 62 70 72 75 80 87 89 90 90a)
    endif()
    
    # CUDA compiler flags
    if(MSVC)
        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -Xcompiler=\"/W3 /wd4819 /wd4244 /wd4251 /wd4267 /wd4275 /wd4068\"")
    else()
        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -Xcompiler -Wall,-Wextra,-Wno-unused-parameter")
    endif()
endif()

# OpenMP specific settings
if(USE_OPENMP)
    if(APPLE)
        if(CMAKE_C_COMPILER_ID MATCHES "Clang")
            set(OpenMP_C_FLAGS "-Xclang -fopenmp")
            set(OpenMP_CXX_FLAGS "-Xclang -fopenmp")
            set(OpenMP_C_LIB_NAMES "libomp")
            set(OpenMP_CXX_LIB_NAMES "libomp")
        endif()
    endif()
endif()
