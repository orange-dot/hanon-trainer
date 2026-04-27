cmake_minimum_required(VERSION 3.25)

project(hanon_trainer VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(PkgConfig REQUIRED)

pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)

set(HT_MIDI_BACKEND "AUTO" CACHE STRING "MIDI backend: AUTO, ALSA, COREMIDI, WINMM, or FAKE")
set_property(CACHE HT_MIDI_BACKEND PROPERTY STRINGS AUTO ALSA COREMIDI WINMM FAKE)
string(TOUPPER "${HT_MIDI_BACKEND}" HT_MIDI_BACKEND_UPPER)

if(HT_MIDI_BACKEND_UPPER STREQUAL "AUTO")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(HT_SELECTED_MIDI_BACKEND "ALSA")
  elseif(APPLE)
    set(HT_SELECTED_MIDI_BACKEND "COREMIDI")
  elseif(WIN32)
    set(HT_SELECTED_MIDI_BACKEND "WINMM")
  else()
    message(FATAL_ERROR "No AUTO MIDI backend for ${CMAKE_SYSTEM_NAME}; set HT_MIDI_BACKEND=FAKE")
  endif()
else()
  set(HT_SELECTED_MIDI_BACKEND "${HT_MIDI_BACKEND_UPPER}")
endif()

set(HT_MIDI_HAL_SOURCE "")
set(HT_MIDI_HAL_INCLUDE_DIRS "")
set(HT_MIDI_HAL_LIBRARIES "")

if(HT_SELECTED_MIDI_BACKEND STREQUAL "ALSA")
  pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
  set(HT_MIDI_HAL_SOURCE src/midi_hal_alsa.c)
  set(HT_MIDI_HAL_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})
  set(HT_MIDI_HAL_LIBRARIES PkgConfig::ALSA)
elseif(HT_SELECTED_MIDI_BACKEND STREQUAL "COREMIDI")
  find_library(COREMIDI_LIBRARY CoreMIDI REQUIRED)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
  set(HT_MIDI_HAL_SOURCE src/midi_hal_coremidi.c)
  set(HT_MIDI_HAL_LIBRARIES ${COREMIDI_LIBRARY} ${COREFOUNDATION_LIBRARY})
elseif(HT_SELECTED_MIDI_BACKEND STREQUAL "WINMM")
  set(HT_MIDI_HAL_SOURCE src/midi_hal_winmm.c)
  set(HT_MIDI_HAL_LIBRARIES winmm)
elseif(HT_SELECTED_MIDI_BACKEND STREQUAL "FAKE")
  set(HT_MIDI_HAL_SOURCE src/midi_hal_fake.c)
else()
  message(FATAL_ERROR "Unsupported HT_MIDI_BACKEND=${HT_MIDI_BACKEND}")
endif()

function(ht_apply_warnings name)
  if(MSVC)
    target_compile_options(${name} PRIVATE /W4)
  else()
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

function(ht_apply_unix_definitions name)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(${name} PRIVATE _DEFAULT_SOURCE _POSIX_C_SOURCE=200809L)
  elseif(APPLE)
    target_compile_definitions(${name} PRIVATE _DARWIN_C_SOURCE)
  endif()
endfunction()

add_library(ht_midi_decode STATIC
  src/midi_decode.c
)
target_include_directories(ht_midi_decode
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
ht_apply_warnings(ht_midi_decode)

add_library(ht_midi_hal STATIC
  ${HT_MIDI_HAL_SOURCE}
)
target_include_directories(ht_midi_hal
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${HT_MIDI_HAL_INCLUDE_DIRS}
)
target_link_libraries(ht_midi_hal PRIVATE ${HT_MIDI_HAL_LIBRARIES})
target_compile_definitions(ht_midi_hal PRIVATE HT_MIDI_BACKEND="${HT_SELECTED_MIDI_BACKEND}")
ht_apply_unix_definitions(ht_midi_hal)
ht_apply_warnings(ht_midi_hal)

add_library(hanon_trainer STATIC
  src/advice_orchestrator.c
  src/codex_cli_adapter.c
  src/content_catalog.c
  src/guidance_renderer.c
  src/ht_types.c
  src/midi_capture.c
  src/overlay_store.c
  src/performance_analysis.c
  src/score_renderer.c
  src/session_core.c
  src/sqlite_store.c
)

target_include_directories(hanon_trainer
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

ht_apply_unix_definitions(hanon_trainer)
ht_apply_warnings(hanon_trainer)

target_link_libraries(hanon_trainer
  PRIVATE
    ht_midi_decode
    ht_midi_hal
    PkgConfig::SQLITE3
    PkgConfig::SDL2
    PkgConfig::SDL2_TTF
)

add_executable(ht_midi_probe tools/ht_midi_probe.c)
target_include_directories(ht_midi_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(ht_midi_probe
  PRIVATE
    HT_PROJECT_VERSION="${PROJECT_VERSION}"
)
ht_apply_unix_definitions(ht_midi_probe)
ht_apply_warnings(ht_midi_probe)
target_link_libraries(ht_midi_probe PRIVATE ht_midi_decode ht_midi_hal)

add_executable(ht_viewer_snapshot tools/ht_viewer_snapshot.c)
ht_apply_unix_definitions(ht_viewer_snapshot)
ht_apply_warnings(ht_viewer_snapshot)
target_link_libraries(ht_viewer_snapshot PRIVATE hanon_trainer)

function(ht_add_c_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE hanon_trainer)
  ht_apply_warnings(${name})
  target_compile_definitions(${name} PRIVATE HT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME ${name} COMMAND ${name})
endfunction()

ht_add_c_test(ht_test_catalog tests/test_catalog.c)
ht_add_c_test(ht_test_overlay_store tests/test_overlay_store.c)
ht_add_c_test(ht_test_sqlite_store tests/test_sqlite_store.c)
ht_add_c_test(ht_test_performance_analysis tests/test_performance_analysis.c)
ht_add_c_test(ht_test_hanon_01c_pilot_analysis tests/test_hanon_01c_pilot_analysis.c)
ht_add_c_test(ht_test_score_renderer_layout tests/test_score_renderer_layout.c)
ht_add_c_test(ht_test_score_renderer_ppm tests/test_score_renderer_ppm.c)
ht_add_c_test(ht_test_hanon_01c_viewer_projection tests/test_hanon_01c_viewer_projection.c)
ht_add_c_test(ht_test_advice_orchestrator tests/test_advice_orchestrator.c)
ht_add_c_test(ht_test_platform_boundaries tests/test_platform_boundaries.c)

add_executable(ht_test_midi_decode tests/test_midi_decode.c)
target_link_libraries(ht_test_midi_decode PRIVATE ht_midi_decode)
ht_apply_warnings(ht_test_midi_decode)
add_test(NAME ht_test_midi_decode COMMAND ht_test_midi_decode)

if(HT_SELECTED_MIDI_BACKEND STREQUAL "FAKE")
  add_executable(ht_test_midi_hal_fake tests/test_midi_hal_fake.c)
  target_include_directories(ht_test_midi_hal_fake PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(ht_test_midi_hal_fake PRIVATE hanon_trainer ht_midi_hal)
  ht_apply_warnings(ht_test_midi_hal_fake)
  add_test(NAME ht_test_midi_hal_fake COMMAND ht_test_midi_hal_fake)
endif()

add_test(
  NAME ht_header_independence
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_header_independence.py
    --compiler ${CMAKE_C_COMPILER}
    --include ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(
  NAME ht_identifier_discipline
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_identifier_discipline.py
    --include ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(
  NAME ht_header_graph
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_header_graph.py
    --include ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(
  NAME ht_companion_docs
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_companion_docs.py
    --root ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME ht_midi_probe_cli
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_midi_probe_cli.py
    --probe $<TARGET_FILE:ht_midi_probe>
)

add_test(
  NAME ht_viewer_snapshot_cli
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_viewer_snapshot_cli.py
    --viewer $<TARGET_FILE:ht_viewer_snapshot>
    --fixtures ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/viewer-corpus
    --out-dir ${CMAKE_CURRENT_BINARY_DIR}/viewer-snapshot-cli
)

add_test(
  NAME ht_pc4_fixture_schema
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_pc4_fixture_schema.py
    --out-dir ${CMAKE_CURRENT_BINARY_DIR}/generated-fixtures/synthetic-pc4-capture
    --seed 20260425
    --probe $<TARGET_FILE:ht_midi_probe>
)

add_test(
  NAME ht_pc4_live_midi_fixtures
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_pc4_live_midi_fixtures.py
    --fixtures ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/pc4-live-midi
    --probe $<TARGET_FILE:ht_midi_probe>
)

add_test(
  NAME ht_asset_generation
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/check_asset_generation.py
    --root ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(ht_asset_generation PROPERTIES SKIP_REGULAR_EXPRESSION "^SKIP:")
