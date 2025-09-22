# Find required dependencies
find_package(HDF5 REQUIRED)
message(STATUS "✓ Found HDF5 ${HDF5_VERSION}")

if(USE_OPENMP)
    find_package(FFTW3f REQUIRED)
    message(STATUS "✓ Found FFTW3f ${FFTW3f_VERSION}")
    
    find_package(OpenMP REQUIRED)
    message(STATUS "✓ Found OpenMP ${OpenMP_CXX_VERSION}")
endif()

if(USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "✓ Found CUDA Toolkit ${CUDAToolkit_VERSION}")
endif()