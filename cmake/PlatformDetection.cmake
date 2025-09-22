# Detect operating system
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX ON)
    set(PLATFORM_UNIX ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS ON)
    set(PLATFORM_UNIX ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS ON)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Detect compiler
if(MSVC)
    set(COMPILER_MSVC ON)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(COMPILER_GCC ON)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(COMPILER_CLANG ON)
else()
    message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()


