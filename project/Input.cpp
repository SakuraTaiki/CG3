#include"Input.h"
#include <cassert>


#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

Input::Input() {

}

Input::~Input() {

	if (keyboard) {
		keyboard->Unacquire(); // デバイスの解放
		keyboard->Release();
		keyboard = nullptr;
	}
	if (directInput) {
		directInput->Release();
		directInput = nullptr;
	}
}

void Input::Initialize(WinApp*winApp) {
	//DirectInputの初期化

	HRESULT result = DirectInput8Create(
		GetModuleHandle(nullptr),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&directInput,
		nullptr
	);
	assert(SUCCEEDED(result));

	//キーボードデバイスの生成

	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

	this->winApp = winApp;

};
void Input::Update() 
{
	keyboard->Acquire();

	keyboard->GetDeviceState(sizeof(key), key);
}
bool Input::Pushkey(BYTE keyNumber) 
{
	if (key[keyNumber]) {
		return true;
	}

	return false;
}
