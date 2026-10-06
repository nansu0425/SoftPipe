cmake_minimum_required(VERSION 3.25)
project(SoftPipe LANGUAGES CXX)

add_executable(SoftPipe WIN32
    src/Diagnostics.cpp
    src/Diagnostics.h
    src/FrameTiming.cpp
    src/FrameTiming.h
    src/Main.cpp
    src/SoftPipe.manifest
)

target_compile_features(SoftPipe PRIVATE cxx_std_20)
set_target_properties(SoftPipe PROPERTIES CXX_EXTENSIONS OFF)

target_compile_definitions(SoftPipe PRIVATE
    UNICODE
    _UNICODE
    $<$<NOT:$<CONFIG:Release>>:SOFTPIPE_ASSERT_ENABLED>
)
target_compile_options(SoftPipe PRIVATE /W4 /permissive- /sdl /utf-8)

target_include_directories(SoftPipe SYSTEM PRIVATE third_party)

set_target_properties(SoftPipe PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT SoftPipe)
