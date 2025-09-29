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
if(USE_OPENMP)
    target_link_libraries(kspace_core PUBLIC
        OpenMP::OpenMP_CXX
    )
    # Core uses FFTW (wisdom and threading) so require FFTW when OpenMP is enabled
    target_link_libraries(kspace_core PUBLIC FFTW3::fftw3f FFTW3::fftw3f_threads)
endif()

target_include_directories(kspace_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/platforms
)

# Link HDF5 via unified interface target
target_link_libraries(kspace_core PUBLIC kspace_hdf5)

# Link compiler flags
target_link_libraries(kspace_core PUBLIC kspace_compiler_flags)

# Add platform-specific definitions
if(PLATFORM_LINUX)
    target_compile_definitions(kspace_core PUBLIC __PLATFORM_LINUX__)
elseif(PLATFORM_MACOS)
    target_compile_definitions(kspace_core PUBLIC __PLATFORM_MACOS__)
elseif(PLATFORM_WINDOWS)
    target_compile_definitions(kspace_core PUBLIC __PLATFORM_WINDOWS__)
endif()

# No direct FFTW include propagation from core; backend target links FFTW imported targets
