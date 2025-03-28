include_directories(include)

file(GLOB MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})
target_link_libraries(hal PUBLIC gpiod)


target_include_directories(hal PUBLIC include)
