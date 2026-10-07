#include "Capture.h"
#include "Diagnostics.h"
#include "FrameTiming.h"
#include "ImageView.h"
#include "Presenter.h"
#include "TestPattern.h"

#include <cstdint>
#include <format>
#include <optional>
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

        FillTestPatternR8G8B8A8(app.framebuffer.data(), kRenderWidth, kRenderHeight, app.sceneSeconds);

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

    constexpr LRESULT kHandled = 0;          // 대부분의 message: 처리했으면 0
    constexpr LRESULT kBackgroundErased = 1; // WM_ERASEBKGND: 0 이 아니면 배경을 지운 것으로 간주

    App* GetApp(HWND hwnd)
    {
        return reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    void AttachApp(HWND hwnd, LPARAM createStruct)
    {
        App* app = static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(createStruct)->lpCreateParams);
        app->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }

    LRESULT SkipBackgroundErase()
    {
        return kBackgroundErased;
    }

    LRESULT RepaintLastFrame(App& app)
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(app.hwnd, &ps);
        PresentFramebuffer(app, dc);
        EndPaint(app.hwnd, &ps);
        return kHandled;
    }

    // 창 테두리를 끄는 동안 DefWindowProcW 가 자체 modal loop 를 돌아 RunMessageLoop 가 멈춘다.
    LRESULT StartFramesDuringSizeMove(App& app)
    {
        SetTimer(app.hwnd, kSizeMoveTimerId, USER_TIMER_MINIMUM, nullptr);
        return kHandled;
    }

    LRESULT StopFramesDuringSizeMove(App& app)
    {
        KillTimer(app.hwnd, kSizeMoveTimerId);
        return kHandled;
    }

    std::optional<LRESULT> RunFrameDuringSizeMove(App& app, WPARAM timerId)
    {
        if (timerId != kSizeMoveTimerId)
        {
            return std::nullopt;
        }
        RunFrame(app);
        return kHandled;
    }

    bool IsAutoRepeat(LPARAM keyFlags)
    {
        return (keyFlags & (1 << 30)) != 0;
    }

    std::optional<LRESULT> HandleKeyDown(App& app, WPARAM virtualKey, LPARAM keyFlags)
    {
        if (IsAutoRepeat(keyFlags))
        {
            return std::nullopt;
        }

        switch (virtualKey)
        {
        case VK_F9:
            app.captureRequested = true;
            return kHandled;
        case VK_PAUSE:
            app.paused = !app.paused;
            return kHandled;
        }
        return std::nullopt;
    }

    LRESULT QuitMessageLoop()
    {
        PostQuitMessage(0);
        return kHandled;
    }

    std::optional<LRESULT> HandleMessage(App& app, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_ERASEBKGND:    return SkipBackgroundErase();
        case WM_PAINT:         return RepaintLastFrame(app);
        case WM_ENTERSIZEMOVE: return StartFramesDuringSizeMove(app);
        case WM_EXITSIZEMOVE:  return StopFramesDuringSizeMove(app);
        case WM_TIMER:         return RunFrameDuringSizeMove(app, wParam);
        case WM_KEYDOWN:       return HandleKeyDown(app, wParam, lParam);
        case WM_DESTROY:       return QuitMessageLoop();
        }
        return std::nullopt;
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            AttachApp(hwnd, lParam);
        }

        if (App* app = GetApp(hwnd))
        {
            if (std::optional<LRESULT> result = HandleMessage(*app, msg, wParam, lParam))
            {
                return *result;
            }
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
