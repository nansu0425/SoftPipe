#include "Capture.h"

#include "Diagnostics.h"

#include <cstdint>
#include <format>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>
#include <windows.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>

namespace
{
    constexpr wchar_t kCaptureDirectory[] = L"captures";

    void AppendToStream(void* context, void* data, int size)
    {
        static_cast<std::ofstream*>(context)->write(static_cast<const char*>(data), size);
    }

    std::vector<uint8_t> ToTightRgb(const ImageView& image)
    {
        std::vector<uint8_t> rgb(static_cast<size_t>(image.width) * image.height * 3);
        const auto* sourceRow = static_cast<const uint8_t*>(image.pixels);
        uint8_t* dest = rgb.data();
        for (uint32_t y = 0; y < image.height; ++y)
        {
            for (uint32_t x = 0; x < image.width; ++x)
            {
                const uint8_t* source = sourceRow + x * 4;
                dest[0] = source[0];
                dest[1] = source[1];
                dest[2] = source[2];
                dest += 3;
            }
            sourceRow += image.rowPitch;
        }
        return rgb;
    }

    std::filesystem::path MakeCapturePath()
    {
        SYSTEMTIME now;
        GetLocalTime(&now);

        const std::filesystem::path directory =
            std::filesystem::path(kCaptureDirectory) / std::format(L"{:04}-{:02}-{:02}", now.wYear, now.wMonth, now.wDay);
        const std::wstring stem =
            std::format(L"{:02}{:02}{:02}_{:03}", now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);

        std::filesystem::path path = directory / (stem + L".png");
        for (int suffix = 1; std::filesystem::exists(path); ++suffix)
        {
            path = directory / std::format(L"{}_{}.png", stem, suffix);
        }
        return path;
    }
}

bool WritePngRgb(const std::filesystem::path& path, const ImageView& image)
{
    SOFTPIPE_ASSERT(IsR8G8B8A8(image.format));
    SOFTPIPE_ASSERT(image.rowPitch >= image.width * 4);

    const std::vector<uint8_t> rgb = ToTightRgb(image);
    std::ofstream file(path, std::ios::binary);
    if (!file)
    {
        return false;
    }

    const int width = static_cast<int>(image.width);
    const int height = static_cast<int>(image.height);
    const int written = stbi_write_png_to_func(AppendToStream, &file, width, height, 3, rgb.data(), width * 3);
    return written != 0 && file.good();
}

void SaveCapture(const ImageView& image)
{
    const std::filesystem::path path = MakeCapturePath();

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
    {
        Log(L"capture failed: cannot create {}", path.parent_path().wstring());
        return;
    }

    if (WritePngRgb(path, image))
    {
        Log(L"capture saved: {}", path.wstring());
    }
    else
    {
        Log(L"capture failed: cannot write {}", path.wstring());
    }
}
