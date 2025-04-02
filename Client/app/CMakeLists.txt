include_directories(include)

file(GLOB MY_SOURCES "src/*.c" "src/client.c" "src/thread_manager.c" "src/draw_stuff.c" "src/audioMixer.c" "src/sound_effects.c")

# Rename the executable to "tank_client"
add_executable(tank_client ${MY_SOURCES})

target_link_libraries(tank_client PRIVATE
        hal
        lcd
        lgpio
        pthread
)

# Link ALSA (for audio)
find_package(ALSA REQUIRED)
target_link_libraries(tank_client PRIVATE asound)

# Where to copy the final executable (already defined in your top-level CMake)
set(NFS_TARGET_DIR "$ENV{HOME}/cmpt433/public/myApps/Project")

# Ensure the wave-files folder goes to the same place
set(NFS_TARGET_WAV_DIR "${NFS_TARGET_DIR}/wav-files")

# Copy the main executable
add_custom_command(TARGET tank_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:tank_client> ${NFS_TARGET_DIR}/tank_client
        COMMENT "Copying tank_client to NFS share: ${NFS_TARGET_DIR}"
)

# Copy wave-files
add_custom_command(TARGET tank_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets/wav-files"
        "${NFS_TARGET_WAV_DIR}"
        COMMENT "Copying wav-files to NFS share: ${NFS_TARGET_WAV_DIR}"
)
