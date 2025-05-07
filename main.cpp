#include <Windows.h>
#include<cstdint>　　	//ウィンドウを作ろう6ページから
//Windousアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	////クライエント領域のサイズ
	//const int32_t kClientWidth = 1280;
	//const int32_t kClientHeith = 720;
	////ウィンドウサイズを表す構造体にクライエント領域を入れる
	//RECT wrc = { 0,0,kClientWidth,kClientHeith };
	////クライエント領域を元に実際のサイズにwrcを変更してもらう
	//AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//WNDCLASS wc{};
	////ウィンドウの生成
	//HWND hwnd = CreateWindow(wc.lpszClassName,
	//	L"CG2",
	//	WS_OVERLAPPEDWINDOW,
	//	CW_USEDEFAULT,
	//	CW_USEDEFAULT,
	//	wrc.right - wrc.left,
	//	wrc.bottom - wrc.top, 
	//	nullptr, nullptr,
	//	wc.hInstance, 
	//	nullptr);
	////ウィンドウを表示する
	//ShowWindow(hwnd, SW_SHOW);
	////ウィンドウプロシージャ
	//wc.lpfnWndProc = WindowProc;
	////ウィンドウクラス名	
	//wc.lpszClassName = L"CG2WIndowClass";
	////インスタンスハンドル
	//wc.hInstance = GetModuleHandle(nullptr);
	////カーソル
	//wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	////ウィンドウクラスを登録する
	//RegisterClass(&wc);
	//出力ウィンドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	return 0;
}


//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//ウィンドウが破壊された
	case WM_DESTROY:
		//OSに対してアプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}