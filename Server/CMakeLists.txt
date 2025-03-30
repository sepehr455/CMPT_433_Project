cmake_minimum_required(VERSION 3.10)
project(TankBattleServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")

# Find SFML libraries
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Add the executable
add_executable(TankBattleServer
        main.cpp
        src/GameServer.cpp
        src/GameRender.cpp
        src/GameState.cpp
        src/Enemy.cpp
)

# Include directories
target_include_directories(TankBattleServer PRIVATE include)

# Link SFML libraries and pthread
target_link_libraries(TankBattleServer
        sfml-graphics
        sfml-window
        sfml-system
        pthread
)

# Copy Assets (body.png, turret.png, etc.)
add_custom_command(TARGET TankBattleServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:TankBattleServer>/Assets
        COMMENT "Copying entire Assets folder to build directory"
)


# (Optional) custom run target
add_custom_target(run
        COMMAND ./TankBattleServer
        DEPENDS TankBattleServer
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
