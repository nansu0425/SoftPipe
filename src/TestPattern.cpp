#include "TestPattern.h"

#include "Diagnostics.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr uint32_t kCornerInset = 8;
    constexpr uint32_t kCornerSize = 64;
    constexpr uint32_t kCheckerSize = 256;
    constexpr uint32_t kBarWidth = 16;
    constexpr double kBarPixelsPerSecond = 200.0;

    constexpr uint32_t Rgba(uint32_t r, uint32_t g, uint32_t b)
    {
        return r | (g << 8) | (b << 16) | (0xFFu << 24);
    }

    constexpr uint32_t kBlack = Rgba(0, 0, 0);
    constexpr uint32_t kWhite = Rgba(255, 255, 255);
    constexpr uint32_t kRed = Rgba(255, 0, 0);
    constexpr uint32_t kGreen = Rgba(0, 255, 0);
    constexpr uint32_t kBlue = Rgba(0, 0, 255);
    constexpr uint32_t kYellow = Rgba(255, 255, 0);

    class PixelWriter
    {
    public:
        PixelWriter(uint32_t* pixels, uint32_t width, uint32_t height)
            : m_pixels(pixels), m_width(width), m_height(height)
        {
        }

        uint32_t Width() const { return m_width; }
        uint32_t Height() const { return m_height; }

        uint32_t* Row(uint32_t y) const
        {
            return m_pixels + static_cast<size_t>(y) * m_width;
        }

        void FillRect(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint32_t color) const
        {
            const uint32_t right = std::min(left + width, m_width);
            const uint32_t bottom = std::min(top + height, m_height);
            for (uint32_t y = top; y < bottom; ++y)
            {
                std::fill(Row(y) + left, Row(y) + right, color);
            }
        }

    private:
        uint32_t* m_pixels;
        uint32_t m_width;
        uint32_t m_height;
    };

    void FillGradient(const PixelWriter& writer)
    {
        const uint32_t maxX = std::max(writer.Width() - 1, 1u);
        const uint32_t maxY = std::max(writer.Height() - 1, 1u);
        for (uint32_t y = 0; y < writer.Height(); ++y)
        {
            uint32_t* row = writer.Row(y);
            for (uint32_t x = 0; x < writer.Width(); ++x)
            {
                row[x] = Rgba(x * 255 / maxX, y * 255 / maxY, 96);
            }
        }
    }

    void FillChecker(const PixelWriter& writer)
    {
        const uint32_t left = (writer.Width() - std::min(kCheckerSize, writer.Width())) / 2;
        const uint32_t top = (writer.Height() - std::min(kCheckerSize, writer.Height())) / 2;
        const uint32_t right = std::min(left + kCheckerSize, writer.Width());
        const uint32_t bottom = std::min(top + kCheckerSize, writer.Height());
        for (uint32_t y = top; y < bottom; ++y)
        {
            uint32_t* row = writer.Row(y);
            for (uint32_t x = left; x < right; ++x)
            {
                row[x] = ((x + y) % 2 == 0) ? kWhite : kBlack;
            }
        }
    }

    void FillCorners(const PixelWriter& writer)
    {
        const uint32_t right = writer.Width() - kCornerInset - kCornerSize;
        const uint32_t bottom = writer.Height() - kCornerInset - kCornerSize;
        writer.FillRect(kCornerInset, kCornerInset, kCornerSize, kCornerSize, kRed);
        writer.FillRect(right, kCornerInset, kCornerSize, kCornerSize, kGreen);
        writer.FillRect(kCornerInset, bottom, kCornerSize, kCornerSize, kBlue);
        writer.FillRect(right, bottom, kCornerSize, kCornerSize, kWhite);
    }

    void FillBorder(const PixelWriter& writer)
    {
        writer.FillRect(0, 0, writer.Width(), 1, kWhite);
        writer.FillRect(0, writer.Height() - 1, writer.Width(), 1, kWhite);
        writer.FillRect(0, 0, 1, writer.Height(), kWhite);
        writer.FillRect(writer.Width() - 1, 0, 1, writer.Height(), kWhite);
    }

    void FillMovingBar(const PixelWriter& writer, double seconds)
    {
        const double travel = writer.Width() + kBarWidth;
        const double offset = std::fmod(seconds * kBarPixelsPerSecond, travel) - kBarWidth;
        const int64_t left = static_cast<int64_t>(std::floor(offset));
        const int64_t clippedLeft = std::max<int64_t>(left, 0);
        const int64_t clippedRight = std::min<int64_t>(left + kBarWidth, writer.Width());
        if (clippedRight > clippedLeft)
        {
            writer.FillRect(
                static_cast<uint32_t>(clippedLeft),
                0,
                static_cast<uint32_t>(clippedRight - clippedLeft),
                writer.Height(),
                kYellow);
        }
    }
}

void FillTestPatternR8G8B8A8(uint32_t* pixels, uint32_t width, uint32_t height, double seconds)
{
    SOFTPIPE_ASSERT(width >= 2 * (kCornerInset + kCornerSize) && height >= 2 * (kCornerInset + kCornerSize));

    const PixelWriter writer(pixels, width, height);
    FillGradient(writer);
    FillChecker(writer);
    FillCorners(writer);
    FillBorder(writer);
    FillMovingBar(writer, seconds);
}
