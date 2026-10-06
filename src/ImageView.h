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

inline bool IsR8G8B8A8(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
}
