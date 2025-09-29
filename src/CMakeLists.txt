# Core library
add_subdirectory(core)

# Backend libraries
if(USE_CUDA)
    add_subdirectory(backends/cuda)
endif()

if(USE_OPENMP)
    add_subdirectory(backends/openmp)
endif()

# Main executable
add_executable(kspaceFirstOrder main.cpp)

# Set output directory
set_target_properties(kspaceFirstOrder PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/Release"
)

# Link core library
target_link_libraries(kspaceFirstOrder PRIVATE kspace_core)

# Link backend-specific libraries
if(USE_CUDA)
    target_link_libraries(kspaceFirstOrder PRIVATE kspace_cuda)
elseif(USE_OPENMP)
    target_link_libraries(kspaceFirstOrder PRIVATE 
        kspace_openmp
        ${FFTW_LIBRARIES}
    )
endif()

# Include directories (backend-specific)
target_include_directories(kspaceFirstOrder PRIVATE 
    $<$<BOOL:${USE_OPENMP}>:${FFTW_INCLUDE_DIRS}>
)
