# Roadmap

목적과 목표 데모는 `README.md` 에 있다.

## 역할 분담

- **[P] 목적**: D3D12 가 fixed-function 또는 API 계약으로 정의하는 pipeline 동작과, 그것을 노출하는 API 형태(PSO desc, resource·view 구조). 직접 구현한다.
- **[B] 보일러플레이트**: pipeline 바깥에서 pipeline 을 실행·확인하는 데 필요한 코드. Claude 가 작성한다.

[B] 코드와 [P] 코드의 접점은 두 곳이다.

- Presentation: [P] 의 render target 이 낸 pixel 배열(`const void* pixels, width, height, rowPitch, format`)을 [B] 가 window 에 표시한다.
- Asset: [B] 의 glTF loader 가 plain CPU struct(vertex attribute 배열, index 배열, material, texture mip chain)를 내고, 그것을 [P] 의 buffer·texture resource 로 옮기는 코드는 [P] API 가 정해진 뒤 맞춘다.

## 참고 문서

D3D12 의 rasterization·shader stage 동작은 D3D11.3 Functional Specification 을 따른다: <https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm>

| 주제 | 절 |
|---|---|
| Rasterization rules (coordinate snapping n.8, top-left rule) | 3.4 |
| Pixel shader derivatives (2x2 quad) | 3.5.7 |
| Input Assembler | 8 |
| Vertex Shader | 9 |
| Rasterizer | 15 |
| Pixel Shader | 16 |
| Output Merger | 17 |

## Milestones

각 milestone 은 화면 또는 dump 이미지로 확인되는 완료 기준을 갖는다. [B] milestone 은 앞선 [P] 진행과 병행할 수 있다.

### M0 [B] 빌드 세팅

- [x] `RelWithDebInfo` build preset 추가. 일상 실행은 이 preset 으로 한다
- [x] 외부 header 는 `third_party/` 에 vendoring 하고 `/external:I` + `/external:W0` 로 연결
- [x] `assets/` 경로(git 제외)와 `VS_DEBUGGER_WORKING_DIRECTORY`
- [x] assert·log(`OutputDebugStringW`) header

완료 기준: 세 configuration 모두 warning 0 으로 빌드된다.

### M1 [B] 화면 출력 기반

- [x] `PeekMessageW` 기반 실시간 loop, `QueryPerformanceCounter` frame time, title 에 FPS·ms 표시
- [ ] Presentation: pixel 배열을 `StretchDIBits` 로 표시. 내부 해상도와 client 크기 분리, resize 대응
- [ ] 키 입력으로 현재 frame 을 PNG 로 dump (`stb_image_write`)

완료 기준: CPU 로 채운 움직이는 test pattern 이 resize 후에도 깨지지 않고, dump 한 PNG 가 화면과 같다.

### M2 [B] 입력·카메라·수학

- [ ] HLSL 스타일 math header: `float2/3/4`, `float4x4`, `mul`, `dot`, `cross`, `normalize`, `lerp`, `saturate`
- [ ] 키보드 상태, Raw Input mouse delta, 우클릭 중 cursor capture
- [ ] Free camera: WASD·QE 이동, mouse look, view matrix. 규약은 `README.md` 의 "좌표·matrix 규약"

완료 기준: 입력에 따라 title 의 카메라 위치·방향 값이 바뀐다.

### M3 [P] Resource 와 render target

- [ ] Buffer·Texture2D resource, DXGI format 일부(`R8G8B8A8_UNORM`, `R8G8B8A8_UNORM_SRGB`, `D32_FLOAT`)
- [ ] RTV, `ClearRenderTargetView`, M1 presentation 연결
- [ ] sRGB render target 쓰기 시 linear → sRGB encode

완료 기준: clear color 가 화면에 나온다.

### M4 [P] Rasterizer 핵심

- [ ] Viewport transform, n.8 fixed-point snapping
- [ ] Edge function 기반 scan conversion, top-left rule
- [ ] Cull mode, `FrontCounterClockwise`

완료 기준: 공유 edge 를 가진 삼각형 fan 에서 pixel 이 빠지거나 두 번 칠해지지 않는다(pixel 별 기록 횟수를 이미지로 dump 해 확인).

### M5 [P] Input Assembler·Vertex Shader

- [ ] Vertex·index buffer view, input layout(`D3D12_INPUT_ELEMENT_DESC` 대응), topology(list·strip, strip cut)
- [ ] Vertex shader 를 C++ 함수로 호출, constant buffer
- [ ] PSO desc 로 stage 상태를 묶는다
- [ ] Perspective projection matrix (view space → clip space). M2 카메라의 view matrix 와 함께 vertex shader constant 로 넘긴다

완료 기준: M2 카메라로 회전하는 cube 를 둘러볼 수 있다(depth 없이, 앞면만 보이게 배치).

### M6 [P] 보간·Pixel Shader

- [ ] Barycentric 보간, perspective-correct 보간, `SV_Position` 의미
- [ ] Pixel shader 호출, render target 쓰기

완료 기준: vertex color·UV 를 출력하는 cube 에서 원근 왜곡 없이 보간된다(바닥 plane 의 checker 로 확인).

### M7 [P] Clipping

- [ ] Clip space 판정(`-w ≤ x,y ≤ w`, `0 ≤ z ≤ w`), near·far plane clipping, x·y 는 guard band

완료 기준: 카메라가 geometry 안으로 들어가도 깨지거나 crash 하지 않는다.

### M8 [P] Depth·Output Merger

- [ ] Depth buffer, DSV, `DepthFunc`·write mask
- [ ] (선택) Reversed-Z: near 1·far 0 projection + `DepthFunc` `GREATER`. 일반 Z 와 depth 정밀도를 dump 해 비교
- [ ] Blend state(`SrcBlend`·`DestBlend`·`BlendOp`, write mask)

완료 기준: 겹친 cube 들이 올바른 순서로 가려지고, 반투명 plane 이 뒤를 비춘다.

### M9 [B] glTF loader·asset

- [ ] `cgltf` + `stb_image` 로 mesh·index·material(baseColor, normal, metallicRoughness, alpha mode)·texture 로드
- [ ] Texture mip chain 생성
- [ ] Khronos glTF-Sample-Assets 의 DamagedHelmet, Sponza 를 `assets/` 에 받는 절차

완료 기준: 로드 결과 통계(mesh·삼각형·texture 수)가 log 에 나오고, texture mip 을 PNG 로 dump 해 확인된다.

### M10 [P] Texture·Sampler

- [ ] SRV, sampler state(point·linear filter, address mode)
- [ ] sRGB texture sampling 시 sRGB → linear decode (filtering 전)
- [ ] Pixel shader 를 2x2 quad 단위로 실행해 derivative 계산, LOD 선택, mip 보간

완료 기준: 멀어지는 checker plane 에서 mip level 을 색으로 칠했을 때 거리에 따라 단계가 바뀌고, 일반 렌더에서 aliasing 이 줄어든다.

### M11 [P] glTF 데모

- [ ] M9 출력을 M3 resource 로 옮기는 코드
- [ ] glTF material 용 VS·PS, alpha mode(`MASK` 는 PS discard, `BLEND` 는 M8 blend)

완료 기준: DamagedHelmet, 이어서 Sponza 를 free camera 로 돌아다니며 볼 수 있다. README 의 목표 데모 달성.

## 목표 데모 이후 (선택)

- [P] Scissor, depth bias, stencil
- [P] MSAA: sample pattern, coverage, resolve
- [P] Geometry shader, tessellation(HS·tessellator·DS), stream output
- [P] Compute shader: `Dispatch`, thread group, `groupshared`, `GroupMemoryBarrier`
- [P] Mesh·amplification shader
- [P] Root signature·descriptor heap binding model
- [B] 실제 D3D12(WARP)로 같은 장면을 그려 pixel diff 하는 reference renderer
- [B]/[P] 성능: tile 단위 multithreading, SIMD
