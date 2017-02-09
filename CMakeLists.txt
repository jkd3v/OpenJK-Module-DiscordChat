cmake_minimum_required (VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(PNAME "discord_chat")

project (${PNAME})

# The files to use
file(GLOB module_SRC
  "src/*.h"
  "src/*.c"
  "src/*.cpp"
)

add_subdirectory (include/openjk)
add_subdirectory (include/legacy)