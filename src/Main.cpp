#include "Diagnostics.h"
#include "FrameTiming.h"

#include <format>
#include <string>
#include <windows.h>

namespace
{
    constexpr wchar_t kWindowClassName[] = L"SoftPipeWindow";
    constexpr wchar_t kWindowTitle[] = L"SoftPipe";
    constexpr int kClientWidth = 1280;
    constexpr int kClientHeight = 720;
    constexpr double kFrameRateWindowSeconds = 0.5;

    struct App
    {
        HWND hwnd = nullptr;
        FrameTimer timer;
        FrameRateCounter frameRate{ kFrameRateWindowSeconds };
    };

    void RunFrame(App& app)
    {
        app.timer.Tick();

        if (std::optional<FrameRate> rate = app.frameRate.AddFrame(app.timer.DeltaSeconds()))
        {
            std::wstring title = std::format(
                L"{} | {:.0f} fps | {:.2f} ms", kWindowTitle, rate->framesPerSecond, rate->millisecondsPerFrame);
            SetWindowTextW(app.hwnd, title.c_str());
        }
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            App* app = static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            app->hwnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }

        switch (msg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    int RunMessageLoop(App& app)
    {
        for (;;)
        {
            MSG msg = {};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    return static_cast<int>(msg.wParam);
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }

            if (IsIconic(app.hwnd))
            {
                WaitMessage();
                continue;
            }

            RunFrame(app);
        }
    }
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int nCmdShow)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wc))
    {
        Log(L"RegisterClassExW failed: {}", GetLastError());
        return 1;
    }

    constexpr DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rect = { 0, 0, kClientWidth, kClientHeight };
    AdjustWindowRectEx(&rect, style, FALSE, 0);

    App app;
    HWND hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        kWindowTitle,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        &app);

    if (!hwnd)
    {
        Log(L"CreateWindowExW failed: {}", GetLastError());
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);

    return RunMessageLoop(app);
}
