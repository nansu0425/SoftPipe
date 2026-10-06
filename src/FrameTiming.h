#pragma once

#include <cstdint>
#include <optional>

class FrameTimer
{
public:
    FrameTimer();

    void Tick();
    double DeltaSeconds() const;
    double TotalSeconds() const;

private:
    double ToSeconds(int64_t ticks) const;

    int64_t m_frequency = 0;
    int64_t m_start = 0;
    int64_t m_previous = 0;
    int64_t m_current = 0;
};

struct FrameRate
{
    double framesPerSecond = 0.0;
    double millisecondsPerFrame = 0.0;
};

class FrameRateCounter
{
public:
    explicit FrameRateCounter(double windowSeconds);

    std::optional<FrameRate> AddFrame(double deltaSeconds);

private:
    double m_windowSeconds = 0.0;
    double m_accumulatedSeconds = 0.0;
    int m_frameCount = 0;
};
