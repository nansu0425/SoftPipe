# SoftPipe

- 목적: Direct3D 12 graphics pipeline의 각 stage를 C++로 구현하여 API가 추상화하는 pipeline 동작을 이해한다
- 목표 데모: glTF model을 free camera로 돌아다니며 볼 수 있는 software renderer

## 개발 환경

- Visual Studio 2026 (Desktop development with C++ workload)
- CMake 3.25 이상
- C++20
- Win32 API

## 좌표·matrix 규약

- Right-handed, +Y up (glTF 와 같음)
- Clip space depth 0..1
- Row-vector: `mul(v, M)`, 변환 합성은 `v * World * View * Proj`
- Matrix 는 row-major 로 저장한다. `M[i]` 는 i 번째 row 다(HLSL 과 같은 의미)
- HLSL 의 기본 matrix packing 은 `column_major` 다. HLSL 로 옮길 때 `-Zpr`·`#pragma pack_matrix(row_major)`·`row_major` 중 하나로 row-major 를 지정한다. 빠뜨리면 compile 오류 없이 transform 결과만 틀린다

## 빌드

Visual Studio 에서는 저장소 폴더를 열면(File → Open → Folder) `CMakePresets.json` 을 읽어 configure 한다. configure preset `default`, build preset `debug`/`relwithdebinfo`/`release` 를 고른다.

| build preset | 용도 | `SOFTPIPE_ASSERT` |
|---|---|---|
| `debug` | 최적화 없이 단계별 debugging | 켜짐 |
| `relwithdebinfo` | 일상 실행. 최적화와 PDB 를 함께 쓴다 | 켜짐 |
| `release` | 성능 측정 | 꺼짐 |

명령줄: `cmake --preset default` 로 configure 한 뒤 `cmake --build --preset <build preset>`.

## 실행

- 프로그램은 저장소 root 를 working directory 로 가정하고 상대 경로로 파일을 연다. Visual Studio debugger 실행은 working directory 가 저장소 root 로 설정되어 있다. 명령줄에서 실행할 때는 저장소 root 에서 실행한다.
- glTF 등 asset 은 저장소 root 의 `assets/` 에 둔다. git 에서 제외된다.
- Log 와 assertion 실패는 `OutputDebugStringW` 로 출력된다. Visual Studio 의 Output 창에서 본다.
- 내부 해상도는 1280x720 고정이다. client 크기가 다르면 비율을 유지해 nearest 로 확대·축소하고 남는 영역은 검게 칠한다.

| 키 | 동작 |
|---|---|
| F9 | 현재 frame 을 `captures/<YYYY-MM-DD>/<HHMMSS_mmm>.png` 로 저장한다. `captures/` 는 git 에서 제외된다 |
| Pause | 장면 시간을 멈추거나 다시 흐르게 한다 |

## 검증

`tools\Verify-Presentation.ps1` 은 SoftPipe 를 실행해 장면을 멈추고, 여러 client 크기와 최소화 후 복원에서 F9 PNG 와 화면(`PrintWindow`)을 pixel 단위로 비교한다. 저장소 root 에서 실행한다.

```
powershell -ExecutionPolicy Bypass -File tools\Verify-Presentation.ps1
powershell -ExecutionPolicy Bypass -Command "& ./tools/Verify-Presentation.ps1 -Exe build/Release/SoftPipe.exe -ClientSizes 1280,720,1000,700"
```

## 진행

로드맵과 역할 분담: `ROADMAP.md`
