#pragma once
#include"dinput.h"
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


//入力
class Input {
public:

	HRESULT result;


	//初期化
	void Initialize(HINSTANCE hinstance,HWND hwnd);
	//更新
	void Update();
	//キーが押されているか判定
	bool IsKeyPressed(BYTE keyCode) const;

	//終了処理
	void Finalize();

private: 
	IDirectInput8* directInput_ = nullptr;
	IDirectInput8* keyboard_ = nullptr;

	BYTE key_[256] = {};

	

};