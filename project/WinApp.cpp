#include "WinApp.h"
#include <cstdint>
#include <mmsystem.h>

// ImGuiなどを使う場合
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "winmm.lib")

void WinApp::Initialize()
{
    // ウィンドウを作る前にDPI対応を有効化
    ImGui_ImplWin32_EnableDpiAwareness();

    HRESULT hr =
        CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED
        );

    wc_.lpfnWndProc = WindowProc;
    wc_.lpszClassName = L"CG2WindowClass";
    wc_.hInstance = GetModuleHandle(nullptr);
    wc_.hCursor =
        LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc_);

    // サイズ変更と最大化を禁止する
    constexpr DWORD windowStyle =
        WS_OVERLAPPED |
        WS_CAPTION |
        WS_SYSMENU |
        WS_MINIMIZEBOX;

    RECT windowRectangle = {
        0,
        0,
        kClientWidth,
        kClientHeight
    };

    AdjustWindowRect(
        &windowRectangle,
        windowStyle,
        FALSE
    );

    hwnd_ = CreateWindow(
        wc_.lpszClassName,
        L"CG2",
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRectangle.right -
        windowRectangle.left,
        windowRectangle.bottom -
        windowRectangle.top,
        nullptr,
        nullptr,
        wc_.hInstance,
        nullptr
    );

    ShowWindow(hwnd_, SW_SHOW);
    timeBeginPeriod(1);
}

void WinApp::Finalize() {
    CloseWindow(hwnd_);
    CoUninitialize();
}

bool WinApp::ProcessMessage()
{
    MSG msg{};

    // 溜まっているメッセージをすべて処理する
    while (PeekMessage(
        &msg,
        nullptr,
        0,
        0,
        PM_REMOVE
    )) {
        if (msg.message == WM_QUIT) {
            return true;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return false;
}

LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return true;
    }

    switch (msg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}