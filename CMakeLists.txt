cmake_minimum_required(VERSION 3.10)
project(snake C)

set(CMAKE_C_STANDARD   99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Release by default for performance; override with -DCMAKE_BUILD_TYPE=Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG   "-g -Wall -Wextra -fsanitize=address")

add_executable(snake
    src/main.c
    src/game.c
    src/render.c
    src/input.c
)

# Link math library (available on all Linux)
target_link_libraries(snake m)

# Put binary next to CMakeLists for easy access
set_target_properties(snake PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
