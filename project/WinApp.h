#pragma once
#include <windows.h>
#include <cstdint>
//#include "externals/imgui/imgui.h"
//#include "externals/imgui/imgui_impl_dx12.h"
//#include "externals/imgui/imgui_impl_win32.h"
//#include "externals/imgui/imgui_impl_win32.cpp"

class WinApp {
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
public:
	void Initialize();
	void Update();
	void Finalize();

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;
	HWND GetHwnd()const { return hwnd; }
	HINSTANCE GetHInstance()const { return wc.hInstance; }

	bool ProcessMessage();

private:
	HWND hwnd = nullptr;
	WNDCLASS wc{};

};
