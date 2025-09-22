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
    target_link_libraries(kspace_openmp PUBLIC ${FFTW3F_LIBRARIES})
    target_include_directories(kspace_openmp PUBLIC ${FFTW3F_INCLUDE_DIRS})

    # Link to core library for shared headers
    target_link_libraries(kspace_openmp PRIVATE kspace_core)
    
    # Link OpenMP if found
    if(OpenMP_CXX_FOUND)
        target_link_libraries(kspace_openmp PUBLIC OpenMP::OpenMP_CXX)
    else()
        # Check if OpenMP is not supported (Apple Clang case)
        if(TARGET kspace_openmp_flags AND "${kspace_openmp_flags}" MATCHES "_OPENMP_NOT_SUPPORTED")
            message(STATUS "OpenMP not supported on this platform - building without threading")
        else()
            message(STATUS "OpenMP package not found - will rely on -fopenmp flags")
        endif()
    endif()

    target_link_libraries(kspace_openmp PUBLIC kspace_openmp_flags)

    # Public include directories
    target_include_directories(kspace_openmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()


