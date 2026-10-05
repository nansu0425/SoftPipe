cmake_minimum_required(VERSION 3.25)
project(SoftPipe LANGUAGES CXX)

add_executable(SoftPipe WIN32
    src/Main.cpp
    src/SoftPipe.manifest
)

target_compile_features(SoftPipe PRIVATE cxx_std_20)
set_target_properties(SoftPipe PROPERTIES CXX_EXTENSIONS OFF)

target_compile_definitions(SoftPipe PRIVATE UNICODE _UNICODE)
target_compile_options(SoftPipe PRIVATE /W4 /permissive- /sdl /utf-8)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT SoftPipe)
