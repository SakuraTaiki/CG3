#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include<wrl.h>
#include<cassert>
#include <Windows.h>
#include"WinApp.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Microsoft::WRL;

//入力
class Input
{
public:
	template<class T>using ComPtr = Microsoft::WRL::ComPtr<T>;
public: 
	


	//HRESULT result;
	//初期化
	void Initialize(WinApp*winApp);
	//更新
	void Update();
	

	bool PushKey(BYTE keyNumber);

	bool TriggerKey(BYTE keyNumber);

private: 
	
	IDirectInput8* directInput = nullptr;

	BYTE key[256] = {};

	BYTE keyPre[256] = {};

	ComPtr<IDirectInputDevice8>keyboard;

	WinApp* winApp = nullptr;


};