cmake_minimum_required(VERSION 3.20)
project(Peco VERSION 0.1.0 LANGUAGES CXX)
include(CTest)

if(NOT WIN32)
    message(FATAL_ERROR "Peco currently requires Windows and the Win32 API.")
endif()

add_library(Peco
    src/core/application.cpp
    src/core/timer.cpp
    src/core/window.cpp
    src/graphics/camera.cpp
    src/graphics/canvas2d.cpp
    src/graphics/image_loader.cpp
    src/graphics/imageL2D.cpp
    src/graphics/mesh.cpp
    src/graphics/renderer3d.cpp
    src/input/input.cpp
)
add_library(Peco::Peco ALIAS Peco)

target_compile_features(Peco PUBLIC cxx_std_17)
target_include_directories(Peco PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_definitions(Peco PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(Peco PUBLIC user32 gdi32 ole32 windowscodecs msimg32)

if(MSVC)
    target_compile_options(Peco PRIVATE /W4 /permissive-)
else()
    target_compile_options(Peco PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(PECO_BUILD_EXAMPLES "Build Peco examples" ON)
if(PECO_BUILD_EXAMPLES)
    add_executable(peco_basic WIN32 examples/basic.cpp)
    target_compile_definitions(peco_basic PRIVATE UNICODE _UNICODE)
    target_link_libraries(peco_basic PRIVATE Peco::Peco)
    if(MINGW)
        target_link_options(peco_basic PRIVATE -municode)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(peco_math_tests tests/math_tests.cpp)
    target_link_libraries(peco_math_tests PRIVATE Peco::Peco)
    add_test(NAME peco_math_tests COMMAND peco_math_tests)

    add_executable(peco_renderer_tests tests/renderer_tests.cpp)
    target_link_libraries(peco_renderer_tests PRIVATE Peco::Peco)
    add_test(NAME peco_renderer_tests COMMAND peco_renderer_tests)
endif()
