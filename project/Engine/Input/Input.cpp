#include "Input.h"
#include <cassert>


void Input::Initialize(WinApp* winApp)
{
    assert(winApp);
    assert(winApp->GetHwnd());
    assert(IsWindow(winApp->GetHwnd()));

    winApp_ =
        winApp;

    HRESULT result =
        DirectInput8Create(
            winApp_->GetHInstance(),
            DIRECTINPUT_VERSION,
            IID_IDirectInput8,
            reinterpret_cast<void**>(
                directInput_.GetAddressOf()
                ),
            nullptr
        );

    assert(SUCCEEDED(result));

    result =
        directInput_->CreateDevice(
            GUID_SysKeyboard,
            keyboard_.GetAddressOf(),
            nullptr
        );

    assert(SUCCEEDED(result));

    result =
        keyboard_->SetDataFormat(
            &c_dfDIKeyboard
        );

    assert(SUCCEEDED(result));

    // 標準的で安全な協調レベル
    result =
        keyboard_->SetCooperativeLevel(
            winApp_->GetHwnd(),
            DISCL_FOREGROUND |
            DISCL_NONEXCLUSIVE
        );

    assert(SUCCEEDED(result));

    result =
        keyboard_->Acquire();

    // 起動時にまだフォーカスがない場合があるため、
    // Acquireの失敗はここではassertしない
    if (FAILED(result)) {
        ZeroMemory(
            key_,
            sizeof(key_)
        );
    }

    result = directInput_->CreateDevice(
        GUID_SysMouse,
        mouse_.GetAddressOf(),
        nullptr
    );
    assert(SUCCEEDED(result));

    result = mouse_->SetDataFormat(&c_dfDIMouse2);
    assert(SUCCEEDED(result));

    result = mouse_->SetCooperativeLevel(
        winApp_->GetHwnd(),
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE
    );
    assert(SUCCEEDED(result));

    mouse_->Acquire();
}


void Input::Update()
{
    memcpy(
        keyPre_,
        key_,
        sizeof(key_)
    );

    HRESULT result =
        keyboard_->GetDeviceState(
            sizeof(key_),
            key_
        );

    if (
        result == DIERR_INPUTLOST ||
        result == DIERR_NOTACQUIRED
        ) {
        result =
            keyboard_->Acquire();

        while (result == DIERR_INPUTLOST) {
            result =
                keyboard_->Acquire();
        }

        if (SUCCEEDED(result)) {
            result =
                keyboard_->GetDeviceState(
                    sizeof(key_),
                    key_
                );
        }
    }

    if (FAILED(result)) {
        ZeroMemory(
            key_,
            sizeof(key_)
        );
    }

    result = mouse_->GetDeviceState(
        sizeof(mouseState_),
        &mouseState_
    );

    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        mouse_->Acquire();
        result = mouse_->GetDeviceState(
            sizeof(mouseState_),
            &mouseState_
        );
    }

    if (FAILED(result)) {
        ZeroMemory(&mouseState_, sizeof(mouseState_));
    }
}

bool Input::PushKey(BYTE keyNumber)
{
    return key_[keyNumber] != 0;
}

bool Input::TriggerKey(BYTE keyNumber)
{
    return
        key_[keyNumber] != 0 &&
        keyPre_[keyNumber] == 0;
}
