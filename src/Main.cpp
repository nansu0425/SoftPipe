#include "Capture.h"
#include "Diagnostics.h"
#include "FrameTiming.h"
#include "ImageView.h"
#include "Presenter.h"
#include "TestPattern.h"

#include <cstdint>
#include <format>
#include <string>
#include <vector>
#include <windows.h>

namespace
{
    constexpr wchar_t kWindowClassName[] = L"SoftPipeWindow";
    constexpr wchar_t kWindowTitle[] = L"SoftPipe";
    constexpr uint32_t kRenderWidth = 1280;
    constexpr uint32_t kRenderHeight = 720;
    constexpr int kInitialClientWidth = static_cast<int>(kRenderWidth);
    constexpr int kInitialClientHeight = static_cast<int>(kRenderHeight);
    constexpr double kFrameRateWindowSeconds = 0.5;
    constexpr UINT_PTR kSizeMoveTimerId = 1;

    struct App
    {
        HWND hwnd = nullptr;
        FrameTimer timer;
        FrameRateCounter frameRate{ kFrameRateWindowSeconds };
        std::vector<uint32_t> framebuffer = std::vector<uint32_t>(static_cast<size_t>(kRenderWidth) * kRenderHeight);
        Presenter presenter;
        double sceneSeconds = 0.0;
        bool paused = false;
        bool captureRequested = false;
    };

    ImageView GetFramebufferView(const App& app)
    {
        ImageView view;
        view.pixels = app.framebuffer.data();
        view.width = kRenderWidth;
        view.height = kRenderHeight;
        view.rowPitch = kRenderWidth * sizeof(uint32_t);
        view.format = Format::R8G8B8A8_UNORM;
        return view;
    }

    void PresentFramebuffer(App& app, HDC dc)
    {
        RECT client;
        GetClientRect(app.hwnd, &client);
        app.presenter.Present(dc, client.right - client.left, client.bottom - client.top, GetFramebufferView(app));
    }

    void UpdateTitle(App& app)
    {
        if (std::optional<FrameRate> rate = app.frameRate.AddFrame(app.timer.DeltaSeconds()))
        {
            std::wstring title = std::format(
                L"{} | {:.0f} fps | {:.2f} ms", kWindowTitle, rate->framesPerSecond, rate->millisecondsPerFrame);
            if (app.paused)
            {
                title += L" | paused";
            }
            SetWindowTextW(app.hwnd, title.c_str());
        }
    }

    void RunFrame(App& app)
    {
        app.timer.Tick();
        if (!app.paused)
        {
            app.sceneSeconds += app.timer.DeltaSeconds();
        }

        FillTestPatternR8G8B8A8(
            app.framebuffer.data(), kRenderWidth, kRenderHeight, kRenderWidth * sizeof(uint32_t), app.sceneSeconds);

        HDC dc = GetDC(app.hwnd);
        PresentFramebuffer(app, dc);
        ReleaseDC(app.hwnd, dc);

        if (app.captureRequested)
        {
            SaveCapture(GetFramebufferView(app));
            app.captureRequested = false;
        }

        UpdateTitle(app);
    }

    App* GetApp(HWND hwnd)
    {
        return reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            App* app = static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            app->hwnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }

        App* app = GetApp(hwnd);
        if (!app)
        {
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        switch (msg)
        {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            PresentFramebuffer(*app, dc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        // 창 테두리를 끄는 동안 DefWindowProcW 가 자체 modal loop 를 돌아 RunMessageLoop 가 멈춘다.
        case WM_ENTERSIZEMOVE:
            SetTimer(hwnd, kSizeMoveTimerId, USER_TIMER_MINIMUM, nullptr);
            return 0;

        case WM_EXITSIZEMOVE:
            KillTimer(hwnd, kSizeMoveTimerId);
            return 0;

        case WM_TIMER:
            if (wParam == kSizeMoveTimerId)
            {
                RunFrame(*app);
                return 0;
            }
            break;

        case WM_KEYDOWN:
        {
            const bool isAutoRepeat = (lParam & (1 << 30)) != 0;
            if (isAutoRepeat)
            {
                break;
            }
            if (wParam == VK_F9)
            {
                app->captureRequested = true;
                return 0;
            }
            if (wParam == VK_PAUSE)
            {
                app->paused = !app->paused;
                return 0;
            }
            break;
        }

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
    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wc))
    {
        Log(L"RegisterClassExW failed: {}", GetLastError());
        return 1;
    }

    constexpr DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rect = { 0, 0, kInitialClientWidth, kInitialClientHeight };
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
