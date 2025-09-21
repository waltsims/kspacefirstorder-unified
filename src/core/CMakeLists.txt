# Core library (always built)
set(CORE_SOURCES
    Containers/MatrixContainer.cpp
    Containers/OutputStreamContainer.cpp
    HDF5/Hdf5File.cpp
    HDF5/Hdf5FileHeader.cpp
    KSpaceFirstOrderSolver.cpp
    Logger.cpp
    MatrixClasses/BaseFloatMatrix.cpp
    MatrixClasses/BaseIndexMatrix.cpp
    MatrixClasses/ComplexMatrix.cpp
    MatrixClasses/IndexMatrix.cpp
    MatrixClasses/RealMatrix.cpp
    OutputStreams/BaseOutputStream.cpp
    OutputStreams/CuboidOutputStream.cpp
    OutputStreams/IndexOutputStream.cpp
    OutputStreams/WholeDomainOutputStream.cpp
    CommandLineParameters.cpp
    Parameters.cpp
)

set(CORE_HEADERS
    Containers/MatrixContainer.h
    Containers/MatrixRecord.h
    Containers/OutputStreamContainer.h
    HDF5/Hdf5File.h
    HDF5/Hdf5FileHeader.h
    KSpaceFirstOrderSolver.h
    ErrorMessages.h
    Logger.h
    OutputMessages.h
    MatrixClasses/BaseFloatMatrix.h
    MatrixClasses/BaseIndexMatrix.h
    MatrixClasses/BaseMatrix.h
    MatrixClasses/ComplexMatrix.h
    MatrixClasses/IndexMatrix.h
    MatrixClasses/RealMatrix.h
    OutputStreams/BaseOutputStream.h
    OutputStreams/CuboidOutputStream.h
    OutputStreams/IndexOutputStream.h
    OutputStreams/WholeDomainOutputStream.h
    CommandLineParameters.h
    Parameters.h
    Utils/DimensionSizes.h
    Utils/TimeMeasure.h
)

# Create core library
add_library(kspace_core ${CORE_SOURCES} ${CORE_HEADERS})
set_target_properties(kspace_core PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

# Link interface libraries to core
target_link_libraries(kspace_core PUBLIC 
    kspace_compiler_flags
)
if(USE_CUDA)
    # Expose CUDA headers (e.g., cufft.h) to any target including core headers
    target_link_libraries(kspace_core PUBLIC CUDA::cufft CUDA::cudart)
endif()
if(USE_OPENMP)
    target_link_libraries(kspace_core PUBLIC
        kspace_openmp_flags
    )
endif()

target_include_directories(kspace_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/platforms
    ${HDF5_INCLUDE_DIRS}
)

# Link HDF5 to core library
target_link_libraries(kspace_core PUBLIC ${HDF5_LIBRARIES})

# Link compiler flags
target_link_libraries(kspace_core PUBLIC kspace_compiler_flags)

# Link OpenMP flags if enabled
if(USE_OPENMP AND TARGET kspace_openmp_flags)
    target_link_libraries(kspace_core PUBLIC kspace_openmp_flags)
endif()

# Add platform-specific definitions
if(PLATFORM_LINUX)
    target_compile_definitions(kspace_core PUBLIC __PLATFORM_LINUX__)
elseif(PLATFORM_MACOS)
    target_compile_definitions(kspace_core PUBLIC __PLATFORM_MACOS__)
elseif(PLATFORM_WINDOWS)
    target_compile_definitions(kspace_core PUBLIC __PLATFORM_WINDOWS__)
endif()

# Add FFTW includes if using OpenMP
if(USE_OPENMP)
    if(FFTW_FOUND)
        target_include_directories(kspace_core PUBLIC ${FFTW_INCLUDE_DIRS})
    else()
        # FFTW should be available when USE_OPENMP is ON
        message(FATAL_ERROR "FFTW3 not found but required for OpenMP backend")
    endif()
endif()
