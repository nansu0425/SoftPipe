#pragma once

#include <cstdint>
#include <dxgiformat.h>

struct ImageView
{
    const void* pixels = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t rowPitch = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};
