cmake_minimum_required(VERSION 3.28)

include(${CMAKE_CURRENT_LIST_DIR}/tools/rp6502.cmake)

project(OREGON-TRAIL C CXX ASM)

add_executable(oregon)
rp6502_align(oregon "XRAM_.*")
rp6502_asset(oregon help src/help.txt)
rp6502_asset(oregon talk src/talk.txt)
rp6502_asset(oregon msg src/messages.dat)
rp6502_executable(oregon DATA default RESET default)
rp6502_web(oregon CONFIG [[
    title: 'The Oregon Trail',
    footer: 'The Picocomputer 6502 was created by <a href="https://picocomputer.github.io/index.html">Rumbledethumps</a>.',
]])
target_sources(oregon PRIVATE
    src/game.c
    src/main.c
    src/msg.c
    src/scene.c
    src/talk.c
    src/ui.c
)
