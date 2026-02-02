#pragma once
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include<wrl.h>
#include"WinApp.h"

//入力
class Input
{
public:
	template<class T>using ComPtr = Microsoft::WRL::ComPtr<T>;
public: 
	
	Input();
	~Input();
	//初期化
	void Initialize(WinApp*winApp);
	//更新
	void Update();
	
	bool Pushkey(BYTE keyNumber);

private: 
	
	IDirectInput8* directInput = nullptr;

	BYTE key[256] = {};

	ComPtr<IDirectInputDevice8>keyboard;

	WinApp* winApp = nullptr;


};