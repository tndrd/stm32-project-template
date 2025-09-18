cmake_minimum_required(VERSION 3.20)
project(drive_testbench CXX)

set(CMAKE_CXX_STANDARD 17)

# Using glob to fetch sources is generally a harmful practice
# But anyway let's just make it work and then make it pretty
file(GLOB_RECURSE DRIVE_TB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  motors/*.cpp 
  motors/*.h)

add_library(drive_testbench_main ${DRIVE_TB_SOURCES})
target_link_libraries(drive_testbench_main PUBLIC stm32-plus-plus)
target_include_directories(drive_testbench_main PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)