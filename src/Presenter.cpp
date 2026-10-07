#include "Presenter.h"

#include "Diagnostics.h"

#include <cstring>

namespace
{
    uint32_t SwapRedBlue(uint32_t rgba)
    {
        return (rgba & 0xFF00FF00u) | ((rgba & 0x000000FFu) << 16) | ((rgba >> 16) & 0x000000FFu);
    }

    int64_t RoundedDivide(int64_t numerator, int64_t denominator)
    {
        return (numerator + denominator / 2) / denominator;
    }

    void FillBlack(HDC dc, int left, int top, int width, int height)
    {
        if (width > 0 && height > 0)
        {
            PatBlt(dc, left, top, width, height, BLACKNESS);
        }
    }

    void FillOutside(HDC dc, int clientWidth, int clientHeight, const RECT& inner)
    {
        FillBlack(dc, 0, 0, clientWidth, inner.top);
        FillBlack(dc, 0, inner.bottom, clientWidth, clientHeight - inner.bottom);
        FillBlack(dc, 0, inner.top, inner.left, inner.bottom - inner.top);
        FillBlack(dc, inner.right, inner.top, clientWidth - inner.right, inner.bottom - inner.top);
    }
}

RECT ComputeLetterboxRect(int clientWidth, int clientHeight, uint32_t imageWidth, uint32_t imageHeight)
{
    const int64_t cw = clientWidth;
    const int64_t ch = clientHeight;
    const int64_t iw = imageWidth;
    const int64_t ih = imageHeight;

    int64_t width = cw;
    int64_t height = ch;
    const bool isWidthLimiting = cw * ih <= ch * iw;
    if (isWidthLimiting)
    {
        height = RoundedDivide(cw * ih, iw);
    }
    else
    {
        width = RoundedDivide(ch * iw, ih);
    }

    const int64_t left = (cw - width) / 2;
    const int64_t top = (ch - height) / 2;
    return RECT{ static_cast<LONG>(left), static_cast<LONG>(top), static_cast<LONG>(left + width), static_cast<LONG>(top + height) };
}

void Presenter::Present(HDC dc, int clientWidth, int clientHeight, const ImageView& image)
{
    SOFTPIPE_ASSERT(IsR8G8B8A8(image.format));
    SOFTPIPE_ASSERT(image.rowPitch >= image.width * 4);

    if (clientWidth <= 0 || clientHeight <= 0 || image.width == 0 || image.height == 0)
    {
        return;
    }

    ConvertToBgrx(image);

    const RECT dest = ComputeLetterboxRect(clientWidth, clientHeight, image.width, image.height);
    FillOutside(dc, clientWidth, clientHeight, dest);

    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = static_cast<LONG>(image.width);
    info.bmiHeader.biHeight = -static_cast<LONG>(image.height);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(
        dc,
        dest.left,
        dest.top,
        dest.right - dest.left,
        dest.bottom - dest.top,
        0,
        0,
        static_cast<int>(image.width),
        static_cast<int>(image.height),
        m_bgrx.data(),
        &info,
        DIB_RGB_COLORS,
        SRCCOPY);
}

void Presenter::ConvertToBgrx(const ImageView& image)
{
    m_bgrx.resize(static_cast<size_t>(image.width) * image.height);

    const auto* sourceRow = static_cast<const uint8_t*>(image.pixels);
    uint32_t* destRow = m_bgrx.data();
    for (uint32_t y = 0; y < image.height; ++y)
    {
        for (uint32_t x = 0; x < image.width; ++x)
        {
            uint32_t rgba;
            std::memcpy(&rgba, sourceRow + x * 4, sizeof(rgba));
            destRow[x] = SwapRedBlue(rgba);
        }
        sourceRow += image.rowPitch;
        destRow += image.width;
    }
}
