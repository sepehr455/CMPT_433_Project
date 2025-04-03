cmake_minimum_required(VERSION 3.10)
project(TankClient VERSION 1.0 DESCRIPTION "Tank Client Project" LANGUAGES C)

# Set the cross-compilation target for ARM64
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)

# Compiler options (applied globally)
set(CMAKE_C_STANDARD 11)

# Always enable AddressSanitizer in Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # AddressSanitizer flags
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)

    # For better stack traces
    add_compile_options(-g)

    message(STATUS "AddressSanitizer enabled (Debug build)")
else()
    # Regular optimization flags for Release builds
    add_compile_options(-Wall -Werror -Wpedantic -Wextra -O2)
    message(STATUS "AddressSanitizer disabled (Release build)")
endif()

# Enable pthread for multithreading support
add_compile_options(-pthread)
add_link_options(-pthread)

# Define the target directory for copying the executable
set(NFS_TARGET_DIR "$ENV{HOME}/cmpt433/public/myApps/Project")

# Ensure the NFS directory exists, create it if necessary
execute_process(COMMAND mkdir -p ${NFS_TARGET_DIR})

# Add subdirectories for hal and app
add_subdirectory(lcd)
add_subdirectory(hal)
add_subdirectory(lgpio)
add_subdirectory(app)

