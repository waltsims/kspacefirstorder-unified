# OpenMP backend library
if(USE_OPENMP)
    # Collect OpenMP source files
    set(OPENMP_SOURCES
        MatrixClasses/FftwComplexMatrix.cpp
        MatrixClasses/FftwRealMatrix.cpp
    )

    # Collect OpenMP header files
    set(OPENMP_HEADERS
        MatrixClasses/FftwComplexMatrix.h
        MatrixClasses/FftwRealMatrix.h
    )

    # Create OpenMP library
    add_library(kspace_openmp ${OPENMP_SOURCES} ${OPENMP_HEADERS})
    target_include_directories(kspace_openmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Link FFTW libraries via imported targets (required by FindDependencies)
    target_link_libraries(kspace_openmp PUBLIC FFTW3::fftw3f FFTW3::fftw3f_threads)

    # Link to core library for shared headers
    target_link_libraries(kspace_openmp PRIVATE kspace_core)
    
    # Link OpenMP if found
    target_link_libraries(kspace_openmp PUBLIC OpenMP::OpenMP_CXX)

    # Public include directories
    target_include_directories(kspace_openmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

