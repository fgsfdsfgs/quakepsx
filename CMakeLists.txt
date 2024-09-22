cmake_minimum_required(VERSION 3.21)

project(quakepsx LANGUAGES C ASM)

file(GLOB _sources src/*.c src/progs/*.c src/*.s)

include_directories(src)

add_compile_options(-flto)
add_link_options(-flto)

get_target_property(OLD_OPTS psn00bsdk INTERFACE_COMPILE_OPTIONS)
string(REPLACE "-O2" "-O3" NEW_OPTS "${OLD_OPTS}")
set_target_properties(psn00bsdk PROPERTIES INTERFACE_COMPILE_OPTIONS "${NEW_OPTS}")

psn00bsdk_add_executable(quakepsx GPREL ${_sources})
psn00bsdk_add_cd_image(quakepsx_iso quakepsx iso.xml DEPENDS quakepsx)
