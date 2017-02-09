project (legacy_${PNAME})

add_definitions(-DMPP_LEGACY)
include_directories(. ../mpp ../util)
add_library(legacy_${PNAME} SHARED ${module_SRC} )