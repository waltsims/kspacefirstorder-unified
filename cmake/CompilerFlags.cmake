add_library(kspace_compiler_flags INTERFACE)

# Common compiler flags
if(COMPILER_MSVC)
    target_compile_options(kspace_compiler_flags INTERFACE
        /W4
        $<$<CONFIG:RELEASE>:/O2>
        $<$<CONFIG:DEBUG>:/Zi>
    )
    # MSVC-specific warning suppressions
    target_compile_definitions(kspace_compiler_flags INTERFACE -D_CRT_SECURE_NO_WARNINGS)

else() # GCC/Clang
    target_compile_options(kspace_compiler_flags INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:RELEASE>:-O3>
        $<$<CONFIG:DEBUG>:-g>
    )
    if(ENABLE_NATIVE_ARCH)
        target_compile_options(kspace_compiler_flags INTERFACE -march=native)
    endif()
endif()

# Backend-specific defines
if(USE_CUDA)
    target_compile_definitions(kspace_compiler_flags INTERFACE -DUSE_CUDA)
    target_compile_options(kspace_compiler_flags INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-O3,--restrict>)
endif()

if(USE_OPENMP)
    target_compile_definitions(kspace_compiler_flags INTERFACE -DUSE_OPENMP)
endif()

# Platform-specific flags (macOS)
if(PLATFORM_MACOS)
    target_compile_options(kspace_compiler_flags INTERFACE
        $<$<COMPILE_LANGUAGE:C>:-Wno-c11-c23-compat>
        -Wno-redundant-decls
        -Wno-float-equal
    )
endif()


