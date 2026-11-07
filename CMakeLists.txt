cmake_minimum_required(VERSION 3.28)

include(${CMAKE_CURRENT_LIST_DIR}/tools/rp6502.cmake)

project(OREGON-TRAIL C CXX ASM)

add_executable(oregon)
rp6502_align(oregon "XRAM_.*")
rp6502_asset(oregon help src/help.txt)
rp6502_asset(oregon talk src/talk.txt)
rp6502_asset(oregon msg src/messages.dat)
rp6502_asset(oregon scenes src/scenes.dat)
rp6502_asset(oregon tunes src/tunes.dat)
rp6502_asset(oregon landmarks src/landmarks.dat)
rp6502_asset(oregon events src/events.dat)
rp6502_executable(oregon DATA default RESET default)
execute_process(
    COMMAND git describe --tags --always
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_VARIABLE OREGON_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE OREGON_VERSION_RESULT)
if(NOT OREGON_VERSION_RESULT EQUAL 0 OR OREGON_VERSION STREQUAL "")
    set(OREGON_VERSION "dev")
endif()
rp6502_web(oregon CONFIG "    title: 'The Oregon Trail',\n    footer: 'Version ${OREGON_VERSION}. The Picocomputer 6502 was created by <a href=\"https://picocomputer.github.io/index.html\">Rumbledethumps</a>.',\n    autoplay: 'off',\n")
target_sources(oregon PRIVATE
    src/game.c
    src/main.c
    src/msg.c
    src/scene.c
    src/sound.c
    src/talk.c
    src/ui.c
)

# The linker checks the static RAM sections, but not the stack. Fail the build
# when the stack would be too short to run safely. The ROM is written first,
# so it can still be inspected.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(oregon_mos_bin "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(OREGON_LLVM_NM llvm-nm HINTS "${oregon_mos_bin}" REQUIRED)
find_program(OREGON_LLVM_SIZE llvm-size HINTS "${oregon_mos_bin}" REQUIRED)
set(OREGON_MIN_STACK 512 CACHE STRING "Bytes of RAM the ROM must leave for the stack")
add_custom_command(TARGET oregon POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/check_memory.py"
            "$<TARGET_FILE:oregon>.elf" "${OREGON_LLVM_NM}" "${OREGON_LLVM_SIZE}" "${OREGON_MIN_STACK}"
    VERBATIM
)
