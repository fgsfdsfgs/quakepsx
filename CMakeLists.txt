cmake_minimum_required(VERSION 3.21)

project(quakepsx LANGUAGES C ASM)

file(GLOB _sources src/*.c src/game/*.c src/*.s)

include_directories(src)

psn00bsdk_add_executable(quakepsx GPREL ${_sources})
psn00bsdk_add_cd_image(quakepsx_iso quakepsx iso.xml DEPENDS quakepsx)
