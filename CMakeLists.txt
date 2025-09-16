cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE common/toolchain.cmake)

# Disable linkning in compiler checks
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(stm32-build C CXX ASM)

include(workspace/project.cmake)

include(devices/${DEVICE_NAME}/options.cmake)
include_directories(lib/CMSIS/CMSIS/Core/Include ${DEVICE_CMSIS_INCLUDE})

set(DEVICE_COMPILE_FLAGS "  \
  -mcpu=${DEVICE_CORE}      \
  -mfpu=${DEVICE_FPU}       \
  -D${DEVICE_FAMILY}        \
  -mthumb                   \
  -lc -lm -lstdc++ -lsupc++ \
")

set(CMAKE_EXE_LINKER_FLAGS "                                     \
  -T ${CMAKE_SOURCE_DIR}/devices/${DEVICE_NAME}/linker_script.ld \
  --specs=nosys.specs                                            \
")

set(CMAKE_C_FLAGS ${DEVICE_COMPILE_FLAGS})
set(CMAKE_CXX_FLAGS ${DEVICE_COMPILE_FLAGS})

add_subdirectory(workspace/)

if(CUSTOM_SYSTEM_INIT)
  set(SYSTEM_INIT_SOURCE "")
else()
  set(SYSTEM_INIT_SOURCE ${DEVICE_SYSTEM_SRC})
endif()

message(INFO CUSTOM_SYSTEM_INIT=${CUSTOM_SYSTEM_INIT})

set(CMAKE_EXECUTABLE_SUFFIX .elf)
add_executable(${ELF_OUTPUT}
  common/syscalls.c
  common/sysmem.c
  ${DEVICE_STARTUP_SRC}
  ${SYSTEM_INIT_SOURCE}
)

target_link_libraries(${ELF_OUTPUT} PRIVATE main)