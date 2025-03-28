include_directories(include)

file(GLOB MY_SOURCES "src/*.c" "src/client.c" src/thread_manager.c)

# Rename the executable to "tank_client"
add_executable(tank_client ${MY_SOURCES})

target_link_libraries(tank_client PRIVATE hal)

# Change the target directory to "myApps/Project"
set(NFS_TARGET_DIR "$ENV{HOME}/cmpt433/public/myApps/Project")

# Ensure only this CMakeLists.txt handles copying the executable
add_custom_command(TARGET tank_client
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:tank_client> ${NFS_TARGET_DIR}/tank_client
        COMMENT "Copying tank_client to NFS share: ${NFS_TARGET_DIR}"
)
