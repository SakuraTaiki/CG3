#pragma once
#include <windows.h>
#include <cstdint>

class WinApp {
public:

	HWND GetHwnd()const { return hwnd; }

	HINSTANCE GetHInstance()const { return wc.hInstance; }

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

	bool ProcessMessage();

	void Initialize();
	void Update();
	
	void Finalize();

private:
	HWND hwnd = nullptr;
	WNDCLASS wc{};
};
