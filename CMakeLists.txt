cmake_minimum_required(VERSION 2.8) 

project(TER CXX)

add_subdirectory(mcgyver)
add_definitions(-DGLM_SWIZZLE)

set(TER_HEADERS include/pov.h)
set(TER_SOURCES src/main.cpp src/pov.cpp)

get_target_property(INCLUDE_DIRS McGyver INCLUDE_DIRECTORIES)
include_directories(include ${INCLUDE_DIRS})

add_executable(capture ${TER_SOURCES} ${TER_HEADERS})
target_link_libraries(capture McGyver ${McGyver_LIBRARIES})
set_property(TARGET capture PROPERTY CXX_STANDARD 11)
