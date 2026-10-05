# SoftPipe

- 프로젝트 목적·개발 환경: `README.md`
- 코드를 바꾼 뒤에는 `cmake --build --preset debug` 로 빌드해 warning 0 을 확인한다. `build/` 가 없으면 먼저 `cmake --preset default` 로 configure 한다.
- 빌드가 link 단계에서 `LNK1168` 로 실패하면 사용자가 `SoftPipe.exe` 를 실행 중(주로 Visual Studio 디버깅)인 것이다. 프로세스를 종료하지 말고 사용자에게 알린다.
