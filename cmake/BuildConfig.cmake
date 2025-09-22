# Set C++ standard, required for all builds
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CUDA specific settings
if(USE_CUDA)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 50 52 53 60 61 62 70 72 75 80 87 89 90 90a)
    endif()

    # CUDA compiler flags
    if(COMPILER_MSVC)
        list(APPEND CUDA_NVCC_FLAGS "-Xcompiler=\"/W3 /wd4819 /wd4244 /wd4251 /wd4267 /wd4275 /wd4068\"")
    else()
        list(APPEND CUDA_NVCC_FLAGS "-Xcompiler=-Wall,-Wextra,-Wno-unused-parameter")
    endif()
    
    # Propagate CUDA flags
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${CUDA_NVCC_FLAGS}")
endif()
