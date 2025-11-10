#include"WinApp.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_win32.cpp"

LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//windowsが放棄された
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	//標準メッセージを行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}


void WinApp::Initialize() {

	//comの初期化
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	WNDCLASS wc{};

	//windowプロシージャ
	wc.lpfnWndProc = WindowProc;

	//windowクラス名
	wc.lpszClassName = L"CG2WindowClass";

	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);

	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//windowクラスを登録する
	RegisterClass(&wc);

	
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//windowsの生成
	
	L"CG2", // タイトルバーの文字
		WS_OVERLAPPEDWINDOW, // よく見るウィンドウタイトル
		CW_USEDEFAULT, // 表示X座標
		CW_USEDEFAULT, // 表示Y座標
		wrc.right - wrc.left, // ウィンドウ横幅
		wrc.bottom - wrc.top, // ウィンドウ縦幅
		nullptr, // 親ウィンドウハンドル
		nullptr, // メニューハンドル
		wc.hInstance, // インスタンスハンドル
		nullptr; // オプション

	ShowWindow(hwnd, SW_SHOW);

}
void WinApp::Update() {

}
void WinApp::Finalize() {
	CloseWindow(hwnd);
	//comの終了時
	CoUninitialize();
}
bool WinApp::ProcessMessage() {
	MSG msg{};
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (msg.message == WM_QUIT) 
	{
		return true;
	}
	return false;
}