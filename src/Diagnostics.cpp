#include "Diagnostics.h"

#include <windows.h>

void WriteDebugOutput(const std::wstring& text)
{
    OutputDebugStringW(text.c_str());
}

void LogAssertionFailure(const wchar_t* expression, const wchar_t* file, int line)
{
    Log(L"{}({}): assertion failed: {}", file, line, expression);
}

bool IsDebuggerAttached()
{
    return IsDebuggerPresent() != FALSE;
}
