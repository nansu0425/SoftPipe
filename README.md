# SoftPipe

- 목적: Direct3D 12 graphics pipeline의 각 stage를 C++로 구현하여 API가 추상화하는 pipeline 동작을 이해한다
- 목표 데모: glTF model을 free camera로 돌아다니며 볼 수 있는 software renderer

## 개발 환경

- Visual Studio 2026 (Desktop development with C++ workload)
- CMake 3.25 이상
- C++20
- Win32 API

## 빌드

Visual Studio 에서는 저장소 폴더를 열면(File → Open → Folder) `CMakePresets.json` 을 읽어 configure 한다. configure preset `default`, build preset `debug`/`release` 를 고른다.
