cmake_minimum_required(VERSION 3.20)
project(EDF6Spectate VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

if(MSVC)
    add_compile_options(/W4 /WX /utf-8 /permissive- /EHsc)
else()
    # MinGW-w64 (cmake/mingw-w64.cmake): static runtimes, so the DLL needs nothing beside it.
    add_compile_options(-Wall -Wextra -Werror -Wno-unknown-pragmas)
    add_link_options(-static -static-libgcc -static-libstdc++)
    string(APPEND CMAKE_CXX_STANDARD_LIBRARIES " -lwinpthread")
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

set(OUT_DIR "${CMAKE_SOURCE_DIR}/out")
# PluginAPI.h from EDFModLoader (MIT, third_party/EDFModLoader/LICENSE.txt).
set(MODLOADER_INCLUDE "${CMAKE_SOURCE_DIR}/third_party/EDFModLoader")

add_library(SpectateCore STATIC src/spectate.cpp src/log.cpp)
target_include_directories(SpectateCore PUBLIC "${CMAKE_SOURCE_DIR}/src")

add_library(EDF6Spectate SHARED src/plugin.cpp)
target_compile_definitions(EDF6Spectate PRIVATE SPECTATE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    SPECTATE_VERSION_MINOR=${PROJECT_VERSION_MINOR} SPECTATE_VERSION_PATCH=${PROJECT_VERSION_PATCH})
target_include_directories(EDF6Spectate PRIVATE ${MODLOADER_INCLUDE})
target_link_libraries(EDF6Spectate PRIVATE SpectateCore)
if(MSVC)
    target_link_options(EDF6Spectate PRIVATE /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO)
endif()
set_target_properties(EDF6Spectate PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}")

# --- Tests ---
enable_testing()
# The installed game the tests read EDF.dll from (only read). Pass -DEDF6_GAME_DIR=... or set the environment variable.
set(EDF6_GAME_DIR "$ENV{EDF6_GAME_DIR}" CACHE PATH "EARTH DEFENSE FORCE 6 install folder (holds EDF.dll)")
option(EDF6_REQUIRE_GAME "Fail the configure when EDF6_GAME_DIR has no EDF.dll" OFF)
set(EDF_DLL "${EDF6_GAME_DIR}/EDF.dll")
if(NOT EXISTS "${EDF_DLL}")
    if(EDF6_REQUIRE_GAME)
        message(FATAL_ERROR "No EDF.dll in EDF6_GAME_DIR (${EDF6_GAME_DIR}), and EDF6_REQUIRE_GAME is ON")
    endif()
    message(WARNING "No EDF.dll in EDF6_GAME_DIR (${EDF6_GAME_DIR}): the tests labelled `game` report skipped")
endif()
# Tests against the game's code exit with 77 (skipped) without EDF.dll.
function(game_test name)
    add_test(NAME ${name} COMMAND ${ARGN})
    set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77 LABELS game)
endfunction()

add_executable(TargetTests tests/target_test.cpp)
target_link_libraries(TargetTests PRIVATE SpectateCore)
add_test(NAME TargetChoice COMMAND TargetTests)

add_executable(CameraTests tests/camera_test.cpp)
target_link_libraries(CameraTests PRIVATE SpectateCore)
game_test(CameraSwitchWithGameCode CameraTests "${EDF_DLL}")

add_executable(PluginTests tests/plugin_test.cpp)
target_include_directories(PluginTests PRIVATE ${MODLOADER_INCLUDE})
add_dependencies(PluginTests EDF6Spectate)
add_test(NAME PluginRefusesOutsideGame
    COMMAND PluginTests "$<TARGET_FILE:EDF6Spectate>" "${CMAKE_BINARY_DIR}/plugintest/nogame")
game_test(PluginLoadsIntoGame PluginTests "$<TARGET_FILE:EDF6Spectate>" "${CMAKE_BINARY_DIR}/plugintest/game"
    "${EDF_DLL}")
