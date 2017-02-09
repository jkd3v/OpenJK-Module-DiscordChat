project (openjk_${PNAME})

add_definitions(-D_CGAME -DNO_CG_TRAP)
include_directories(. ../mpp  ../util)
add_library(openjk_${PNAME} SHARED ${module_SRC} )