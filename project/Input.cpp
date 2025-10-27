#include"Input.h"
#include<cassert>
#include<wrl.h>


using namespace Microsoft::WRL;

void Input::Initialize(HINSTANCE hInstance,HWND hwnd) {
	//DirectInputの初期化
	ComPtr<IDirectInput8*> directInput = nullptr;
	 result=DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	ComPtr<IDirectInputDevice8*> keyboard;
	 result=directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	//入力データ形式のセット
	result=keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	//排他制御レベルのセット
	result=keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

	
};
void Input::Update() {
	
};