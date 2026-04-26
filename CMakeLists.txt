cmake_minimum_required(VERSION 3.25)

project(hanon_trainer VERSION 0.1.0 LANGUAGES C)

if(NOT UNIX)
  message(FATAL_ERROR "hanon-trainer currently targets Unix-like local desktop systems")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(PkgConfig REQUIRED)

pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(ALSA REQUIRED alsa)

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
    ${SQLITE3_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
)

target_compile_definitions(hanon_trainer PRIVATE _DEFAULT_SOURCE _POSIX_C_SOURCE=200809L)

target_compile_options(hanon_trainer PRIVATE -Wall -Wextra -Wpedantic)

target_link_directories(hanon_trainer
  PRIVATE
    ${SQLITE3_LIBRARY_DIRS}
    ${SDL2_LIBRARY_DIRS}
    ${SDL2_TTF_LIBRARY_DIRS}
    ${ALSA_LIBRARY_DIRS}
)

target_link_libraries(hanon_trainer
  PRIVATE
    ${SQLITE3_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${ALSA_LIBRARIES}
)

add_library(ht_midi_decode STATIC
  src/midi_decode.c
)
target_include_directories(ht_midi_decode
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(ht_midi_decode PRIVATE -Wall -Wextra -Wpedantic)

add_executable(ht_midi_probe tools/ht_midi_probe.c)
target_include_directories(ht_midi_probe PRIVATE ${ALSA_INCLUDE_DIRS})
target_compile_definitions(ht_midi_probe
  PRIVATE
    _DEFAULT_SOURCE
    _POSIX_C_SOURCE=200809L
    HT_PROJECT_VERSION="${PROJECT_VERSION}"
)
target_compile_options(ht_midi_probe PRIVATE -Wall -Wextra -Wpedantic)
target_link_directories(ht_midi_probe PRIVATE ${ALSA_LIBRARY_DIRS})
target_link_libraries(ht_midi_probe PRIVATE ht_midi_decode ${ALSA_LIBRARIES})

function(ht_add_c_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE hanon_trainer)
  target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_definitions(${name} PRIVATE HT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME ${name} COMMAND ${name})
endfunction()

ht_add_c_test(ht_test_catalog tests/test_catalog.c)
ht_add_c_test(ht_test_overlay_store tests/test_overlay_store.c)
ht_add_c_test(ht_test_sqlite_store tests/test_sqlite_store.c)
ht_add_c_test(ht_test_performance_analysis tests/test_performance_analysis.c)
ht_add_c_test(ht_test_advice_orchestrator tests/test_advice_orchestrator.c)
ht_add_c_test(ht_test_platform_boundaries tests/test_platform_boundaries.c)

add_executable(ht_test_midi_decode tests/test_midi_decode.c)
target_link_libraries(ht_test_midi_decode PRIVATE ht_midi_decode)
target_compile_options(ht_test_midi_decode PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ht_test_midi_decode COMMAND ht_test_midi_decode)

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
