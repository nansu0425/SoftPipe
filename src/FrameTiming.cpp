#include "FrameTiming.h"

#include <windows.h>

namespace
{
    int64_t QueryCounter()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return counter.QuadPart;
    }
}

FrameTimer::FrameTimer()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    m_frequency = frequency.QuadPart;
    m_current = QueryCounter();
    m_previous = m_current;
}

void FrameTimer::Tick()
{
    m_previous = m_current;
    m_current = QueryCounter();
}

double FrameTimer::DeltaSeconds() const
{
    return ToSeconds(m_current - m_previous);
}

double FrameTimer::ToSeconds(int64_t ticks) const
{
    return static_cast<double>(ticks) / static_cast<double>(m_frequency);
}

FrameRateCounter::FrameRateCounter(double windowSeconds)
    : m_windowSeconds(windowSeconds)
{
}

std::optional<FrameRate> FrameRateCounter::AddFrame(double deltaSeconds)
{
    m_accumulatedSeconds += deltaSeconds;
    ++m_frameCount;
    if (m_accumulatedSeconds < m_windowSeconds)
    {
        return std::nullopt;
    }

    FrameRate rate;
    rate.framesPerSecond = m_frameCount / m_accumulatedSeconds;
    rate.millisecondsPerFrame = m_accumulatedSeconds * 1000.0 / m_frameCount;
    m_accumulatedSeconds = 0.0;
    m_frameCount = 0;
    return rate;
}
