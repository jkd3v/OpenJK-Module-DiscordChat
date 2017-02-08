cmake_minimum_required (VERSION 2.6)
project (discord_chat)

# add preprocessor definitions
add_definitions(-D_CGAME -DNO_CG_TRAP)

# add include directories
include_directories(include/sdk include/mpp include/util)

# add the executable
file(GLOB module_SRC
  "src/*.h"
  "src/*.c"
)
add_library(discord_chat SHARED ${module_SRC})