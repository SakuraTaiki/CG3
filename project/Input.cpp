#include"Input.h"

void Input::Initialize(WinApp*winApp) {
	//DirectInputの初期化

	//HRESULT result;
	this->winApp = winApp;

	directInput = nullptr;
	HINSTANCE hInstance = winApp->GetHInstance();
	HRESULT result = DirectInput8Create(winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	//入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

};
void Input::Update() 
{

	HRESULT result;

	memcpy(keyPre, key, sizeof(key));

	result = keyboard->Acquire();

	result = keyboard->GetDeviceState(sizeof(key), key);
};
bool Input::PushKey(BYTE keyNumber) 
{
	if (key[keyNumber]) {
		return true;
	}

	return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{

	if (!keyPre[keyNumber] && key[keyNumber]) 
	{
		return true;
	}

	return false;
}