#pragma once

#include <cstdlib>
#include <format>
#include <intrin.h>
#include <string>
#include <utility>

void WriteDebugOutput(const std::wstring& text);
void LogAssertionFailure(const wchar_t* expression, const wchar_t* file, int line);
bool IsDebuggerAttached();

template <typename... Args>
void Log(std::wformat_string<Args...> format, Args&&... args)
{
    std::wstring text = std::format(format, std::forward<Args>(args)...);
    text += L'\n';
    WriteDebugOutput(text);
}

#define SOFTPIPE_WIDEN_IMPL(text) L##text
#define SOFTPIPE_WIDEN(text) SOFTPIPE_WIDEN_IMPL(text)

#ifdef SOFTPIPE_ASSERT_ENABLED
#define SOFTPIPE_ASSERT(condition)                                                              \
    do                                                                                          \
    {                                                                                           \
        if (!(condition))                                                                       \
        {                                                                                       \
            LogAssertionFailure(SOFTPIPE_WIDEN(#condition), SOFTPIPE_WIDEN(__FILE__), __LINE__); \
            if (IsDebuggerAttached())                                                           \
            {                                                                                   \
                __debugbreak();                                                                 \
            }                                                                                   \
            else                                                                                \
            {                                                                                   \
                std::abort();                                                                   \
            }                                                                                   \
        }                                                                                       \
    } while (false)
#else
#define SOFTPIPE_ASSERT(condition) static_cast<void>(sizeof(!(condition)))
#endif
