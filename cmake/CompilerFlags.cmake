# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common flags for all compilers
add_library(kspace_compiler_flags INTERFACE)
target_compile_features(kspace_compiler_flags INTERFACE cxx_std_17)
target_compile_options(kspace_compiler_flags INTERFACE
    # Common warnings for all compilers
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    
    # Optimization flags
    $<$<CONFIG:RELEASE>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>>
    $<$<CONFIG:RELEASE>:$<$<CXX_COMPILER_ID:MSVC>:/O2>>
    $<$<CONFIG:DEBUG>:-g>
)

# Native architecture optimization
if(ENABLE_NATIVE_ARCH)
    if(NOT MSVC)
        target_compile_options(kspace_compiler_flags INTERFACE -march=native)
    endif()
endif()

# Backend-specific defines
if(USE_CUDA)
    target_compile_definitions(kspace_compiler_flags INTERFACE -DUSE_CUDA)
    
    # CUDA compiler settings
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 50 52 53 60 61 62 70 72 75 80 87 89 90 90a)
    endif()
    
    target_compile_options(kspace_compiler_flags INTERFACE
        $<$<COMPILE_LANGUAGE:CUDA>:-O3>
        $<$<COMPILE_LANGUAGE:CUDA>:--restrict>
        $<$<COMPILE_LANGUAGE:CUDA,CXX_COMPILER_ID:MSVC>:-Xcompiler="/W3 /wd4819 /wd4244 /wd4251 /wd4267 /wd4275 /wd4068">
        $<$<COMPILE_LANGUAGE:CUDA,NOT:$<CXX_COMPILER_ID:MSVC>>:-Xcompiler -Wall,-Wextra,-Wno-unused-parameter>
    )
endif()

if(USE_OPENMP)
    target_compile_definitions(kspace_compiler_flags INTERFACE -DUSE_OPENMP)
endif()

# Platform-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(kspace_compiler_flags INTERFACE 
        -DWIN32
        $<$<CXX_COMPILER_ID:MSVC>:-D_CRT_SECURE_NO_WARNINGS>
        $<$<CXX_COMPILER_ID:MSVC>:-DWIN32_LEAN_AND_MEAN>
        $<$<CXX_COMPILER_ID:MSVC>:-DNOMINMAX>
    )
endif()

# macOS-specific warning suppressions for system headers
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_options(kspace_compiler_flags INTERFACE
        -Wno-c11-c23-compat          # Suppress _Float16 warnings
        -Wno-redundant-decls         # Suppress redundant declaration warnings
        -Wno-float-equal             # Suppress float equality warnings from system headers
    )
endif()


