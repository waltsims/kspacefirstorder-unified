# Common flags
add_library(kspace_compiler_flags INTERFACE)
target_compile_options(kspace_compiler_flags INTERFACE
    -Wall
    $<$<CONFIG:RELEASE>:-O3>
    $<$<CONFIG:DEBUG>:-g>
)

# Backend-specific defines
if(USE_CUDA)
    target_compile_definitions(kspace_compiler_flags INTERFACE -DUSE_CUDA)
    target_compile_options(kspace_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-O3,--restrict>)
endif()

if(USE_OPENMP)
    target_compile_definitions(kspace_compiler_flags INTERFACE -DUSE_OPENMP)
endif()

# Platform-specific flags
if(ENABLE_NATIVE_ARCH)
    if(PLATFORM_LINUX OR PLATFORM_MACOS)
        target_compile_options(kspace_compiler_flags INTERFACE -march=native)
    endif()
endif()

# macOS-specific warning suppressions for system headers
if(PLATFORM_MACOS)
    target_compile_options(kspace_compiler_flags INTERFACE
        -Wno-c11-c23-compat          # Suppress _Float16 warnings
        -Wno-redundant-decls         # Suppress redundant declaration warnings
        -Wno-float-equal             # Suppress float equality warnings from system headers
    )
endif()


