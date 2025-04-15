#include <Windows.h>

//Windousアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
{
	//出力ウィンドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");
	
	return 0;
}
////ウィンドウプロシージャ
//LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
//	//メッセージに応じてゲーム固有の処理を行う
//	switch (msg)
//	{
//		//ウィンドウが破壊された
//	case WM_DESTROY:
//	//OSに対してアプリの終了を伝える
//		PostQuitMessage(0);
//		return 0;
//	}
//	//標準のメッセージ処理を行う
//	return DefWindowProc(hwnd, msg, wparam, lparam);
//}