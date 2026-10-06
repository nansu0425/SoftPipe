# third_party

외부 header 를 수정 없이 복사해 둔다. 이 directory 는 `/external:I` 로 include 되어 warning 이 꺼진다(`CMakeLists.txt` 의 `SYSTEM` include).

| 파일 | 출처 | version |
|---|---|---|
| `stb_image_write.h` | <https://github.com/nothings/stb> | v1.16 (commit `2c980bb59875b0d32144a71867fbdebb2f77cd20`) |
