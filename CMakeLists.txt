cmake_minimum_required(VERSION 2.8) 

project(TER CXX)

find_package(Boost 1.58 REQUIRED serialization)

add_subdirectory(engine)
add_definitions(-DGLM_SWIZZLE)

set(CAPTURE_HEADERS include/pov.h)
set(CAPTURE_DEBUG_HEADERS include/pov.h)
set(VDTM_HEADERS include/pov.h)
set(CAPTURE_SOURCES src/capture.cpp src/pov.cpp)
set(CAPTURE_DEBUG_SOURCES src/capture_dbg.cpp src/pov.cpp)
set(VDTM_SOURCES src/vdtm.cpp src/pov.cpp)

get_target_property(INCLUDE_DIRS TrollEngine INCLUDE_DIRECTORIES)
include_directories(include ${INCLUDE_DIRS})

add_executable(capture ${CAPTURE_SOURCES} ${CAPTURE_HEADERS})
add_executable(capture_archive_debug ${CAPTURE_DEBUG_SOURCES} ${CAPTURE_DEBUG_HEADERS})
add_executable(vdtm ${VDTM_SOURCES} ${VDTM_HEADERS})
target_link_libraries(capture TrollEngine ${TrollEngine_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(capture_archive_debug TrollEngine ${TrollEngine_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(vdtm TrollEngine ${TrollEngine_LIBRARIES} ${Boost_LIBRARIES})
set_property(TARGET capture PROPERTY CXX_STANDARD 14)
set_property(TARGET capture_archive_debug PROPERTY CXX_STANDARD 14)
set_property(TARGET vdtm PROPERTY CXX_STANDARD 14)
