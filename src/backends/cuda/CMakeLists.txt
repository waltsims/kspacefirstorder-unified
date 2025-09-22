# CUDA backend library
if(USE_CUDA)
    # Collect CUDA source files
    set(CUDA_SOURCES
        Containers/CudaMatrixContainer.cu
        KSpaceSolver/SolverCudaKernels.cu
        MatrixClasses/TransposeCudaKernels.cu
        OutputStreams/OutputStreamsCudaKernels.cu
        Parameters/CudaDeviceConstants.cu
    )

    # Collect CUDA header files
    set(CUDA_HEADERS
        Containers/CudaMatrixContainer.cuh
        KSpaceSolver/SolverCudaKernels.cuh
        MatrixClasses/TransposeCudaKernels.cuh
        OutputStreams/OutputStreamsCudaKernels.cuh
        Parameters/CudaDeviceConstants.cuh
        Utils/CudaUtils.cuh
    )

    # Create CUDA library
    add_library(kspace_cuda ${CUDA_SOURCES} ${CUDA_HEADERS})
    set_target_properties(kspace_cuda PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
    )
    target_include_directories(kspace_cuda PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(kspace_cuda
        PRIVATE
            kspace_core
        PUBLIC
            CUDA::cudart
            CUDA::cufft
    )
endif()


