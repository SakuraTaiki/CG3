#pragma once

#include <Windows.h>
#include <cstdint>

class WinApp
{
public:
    static constexpr int32_t kClientWidth = 1280;
    static constexpr int32_t kClientHeight = 720;

    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

    void Initialize();
    void Finalize();

    bool ProcessMessage();
    bool RestartExecutable();

    void ToggleFullscreen();

    bool ConsumeResize(
        uint32_t& width,
        uint32_t& height
    );

    HWND GetHwnd() const
    {
        return hwnd_;
    }

    HINSTANCE GetHInstance() const
    {
        return wc_.hInstance;
    }

    int32_t GetWidth() const
    {
        return static_cast<int32_t>(clientWidth_);
    }

    int32_t GetHeight() const
    {
        return static_cast<int32_t>(clientHeight_);
    }

    bool IsFullscreen() const
    {
        return isFullscreen_;
    }

private:
    LRESULT HandleMessage(
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

private:
    HWND hwnd_ = nullptr;
    WNDCLASS wc_{};

    uint32_t clientWidth_ =
        kClientWidth;

    uint32_t clientHeight_ =
        kClientHeight;

    bool resizePending_ = false;
    bool isFullscreen_ = false;

    DWORD windowedStyle_ =
        WS_OVERLAPPEDWINDOW;

    WINDOWPLACEMENT windowedPlacement_{
        sizeof(WINDOWPLACEMENT)
    };
};
