#pragma once

#include "Format.h"

#include <cstdint>

struct ImageView
{
    const void* pixels = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t rowPitch = 0;
    Format format = Format::UNKNOWN;
};

inline bool IsR8G8B8A8(Format format)
{
    return format == Format::R8G8B8A8_UNORM || format == Format::R8G8B8A8_UNORM_SRGB;
}
