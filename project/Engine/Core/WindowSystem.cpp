#include "WindowSystem.h"

void WindowSystem::Initialize() {
    winApp_ = std::make_unique<WinApp>();
    winApp_->Initialize();
}

void WindowSystem::Finalize() {
    winApp_.reset();
}

bool WindowSystem::ProcessMessage() {
    return winApp_->ProcessMessage();
}