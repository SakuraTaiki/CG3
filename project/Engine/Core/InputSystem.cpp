#include "InputSystem.h"

#include "EngineContext.h"
#include "WinApp.h"

void InputSystem::Initialize(WinApp* winApp, EngineContext* context) {
    input_ = std::make_unique<Input>();
    input_->Initialize(winApp);

    // Scene から入力を取得できるように Context へ登録。
    context->SetInput(input_.get());
}

void InputSystem::Finalize() {
    input_.reset();
}

void InputSystem::Update() {
    input_->Update();
}