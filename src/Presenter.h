#pragma once

#include "ImageView.h"

#include <cstdint>
#include <vector>
#include <windows.h>

RECT ComputeLetterboxRect(int clientWidth, int clientHeight, uint32_t imageWidth, uint32_t imageHeight);

class Presenter
{
public:
    void Present(HDC dc, int clientWidth, int clientHeight, const ImageView& image);

private:
    void ConvertToBgrx(const ImageView& image);

    std::vector<uint32_t> m_bgrx;
};
