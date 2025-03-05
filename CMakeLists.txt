cmake_minimum_required(VERSION 3.10)
project(TankBattle VERSION 1.0 LANGUAGES C CXX)

# Global settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Enable warnings and optimizations
add_compile_options(-Wall -Werror -Wpedantic -Wextra -O2)

# Define the cross-compilation target for the client (BeagleBone)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Check if we're building the client (C) or server (C++)
if (${CMAKE_PROJECT_NAME} MATCHES "TankClient")
    set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc CACHE STRING "C Compiler")
endif()

# Enable pthread for multithreading support
add_compile_options(-pthread)
add_link_options(-pthread)

# Add subdirectories for Client and Server
add_subdirectory(Client)
add_subdirectory(Server)
