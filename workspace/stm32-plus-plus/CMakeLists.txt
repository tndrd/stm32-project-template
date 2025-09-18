cmake_minimum_required(VERSION 3.20)
project(stm32-plus-plus CXX ASM)

set(CMAKE_CXX_STANDARD 17)

set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/stm32_mw_lwip)

set (LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${LWIP_DIR}/system"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ethernet"
)

add_subdirectory(lib/stm32_mw_lwip)

# Using glob to fetch sources is generally a harmful practice
# But anyway let's just make it work and then make it pretty
file(GLOB_RECURSE STM32PP_SOURCES src/*.cpp src/*.c src/*.h)

add_library(stm32-plus-plus ${STM32PP_SOURCES})
target_include_directories(stm32-plus-plus PUBLIC
  src/
  lib/stm32_mw_lwip/src/include
  lib/stm32_mw_lwip/system
  src/ethernet # otherwise lwipopts.h is not found
)
target_compile_definitions(stm32-plus-plus PUBLIC STM32F4)
target_link_libraries(stm32-plus-plus PUBLIC lwipcore)