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

    # Link FFTW libraries
    if(FFTW_FOUND)
        target_link_libraries(kspace_openmp PUBLIC ${FFTW_LIBRARIES})
        target_include_directories(kspace_openmp PUBLIC ${FFTW_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "FFTW3 not found but required for OpenMP backend")
    endif()

    # Link to core library for shared headers
    target_link_libraries(kspace_openmp PRIVATE kspace_core)
    
    # Link OpenMP if found
    target_link_libraries(kspace_openmp PUBLIC OpenMP::OpenMP_CXX)

    # Public include directories
    target_include_directories(kspace_openmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()


