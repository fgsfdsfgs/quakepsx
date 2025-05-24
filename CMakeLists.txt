cmake_minimum_required(VERSION 3.21)

project(quakepsx LANGUAGES C ASM)

option(INSTRUMENT_FUNCTIONS "Compile with -finstrument-functions and enable Redux profiler" OFF)
option(ENABLE_LTO "Compile with -flto" ON)
option(WITH_CDDA "Build full image with CD Audio tracks (requires said tracks)" OFF)

file(GLOB _sources src/*.c src/progs/*.c src/*.s)

include_directories(src)

add_compile_options(-Wall -Wextra -Wno-strict-aliasing -Wno-unused-parameter -Wno-sign-compare)

set(OPT_FLAGS
  # replace the default -O2 with -O3
  -O3 -freg-struct-return -fsection-anchors
  # these can apparently result in slower code and they don't seem to make much of a size difference
  -fno-data-sections -fno-function-sections
  # we check for zero division by hand when necessary
  -mno-check-zero-division
  # alignment makes no difference
  -flimit-function-alignment -fno-align-functions -fno-align-labels -fno-align-jumps -fno-align-loops
  # we have no data cache
  --param=l1-cache-line-size=0 --param=l1-cache-size=0 --param=l2-cache-size=0
  # don't care about any data races
  -fallow-store-data-races
  # don't care about this since we do no dynamic linking
  -fno-semantic-interposition
)
# this can't fit into the above list because it has a comma
add_compile_options(-Wa,--strip-local-absolute)

if(INSTRUMENT_FUNCTIONS)
  add_compile_options(-finstrument-functions)
  add_definitions(-DINSTRUMENT_FUNCTIONS)
elseif(ENABLE_LTO)
  add_compile_options(-flto -fwhole-program)
  add_link_options(-flto -fwhole-program)
endif()

if(WITH_CDDA)
  message(STATUS "CD audio enabled.")
  set(ISO_CONFIG "iso_cdda.xml")
else()
  set(ISO_CONFIG "iso.xml")
endif()

get_target_property(OLD_OPTS psn00bsdk INTERFACE_COMPILE_OPTIONS)
string(REPLACE "-O2" "${OPT_FLAGS}" NEW_OPTS "${OLD_OPTS}")
set_target_properties(psn00bsdk PROPERTIES INTERFACE_COMPILE_OPTIONS "${NEW_OPTS}")

psn00bsdk_add_executable(quakepsx GPREL ${_sources})
psn00bsdk_add_cd_image(quakepsx_iso quakepsx ${ISO_CONFIG} DEPENDS quakepsx)
