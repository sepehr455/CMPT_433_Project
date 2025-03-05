cmake_minimum_required(VERSION 3.10)
project(TankBattleServer)

set(CMAKE_CXX_STANDARD 17)

# Add the executable
add_executable(TankBattleServer
        main.cpp
        src/GameServer.cpp
)

# Include directories
target_include_directories(TankBattleServer PRIVATE include)

# Custom target to run the server after building
add_custom_target(run
        COMMAND ./TankBattleServer
        DEPENDS TankBattleServer
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
