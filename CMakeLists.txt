cmake_minimum_required(VERSION 2.8) 

project(TER CXX)

add_subdirectory(mcgyver)
add_definitions(-DGLM_SWIZZLE)

set(CAPTURE_HEADERS include/pov.h)
set(VDTM_HEADERS include/pov.h)
set(CAPTURE_SOURCES src/capture.cpp src/pov.cpp)
set(VDTM_SOURCES src/vdtm.cpp src/pov.cpp)

get_target_property(INCLUDE_DIRS McGyver INCLUDE_DIRECTORIES)
include_directories(include ${INCLUDE_DIRS})

add_executable(capture ${CAPTURE_SOURCES} ${CAPTURE_HEADERS})
add_executable(vdtm ${VDTM_SOURCES} ${VDTM_HEADERS})
target_link_libraries(capture McGyver ${McGyver_LIBRARIES})
target_link_libraries(vdtm McGyver ${McGyver_LIBRARIES})
set_property(TARGET capture PROPERTY CXX_STANDARD 11)
set_property(TARGET vdtm PROPERTY CXX_STANDARD 11)