#include "Engine.h"

void Engine::Initialize() {
    // Window は DirectX / Input の初期化に必要なので最初。
    windowSystem_.Initialize();
    context_.SetWinApp(windowSystem_.GetWinApp());

    // Input は Window に依存する。
    inputSystem_.Initialize(windowSystem_.GetWinApp(), &context_);

    // Graphics も Window に依存する。
    graphicsSystem_.Initialize(windowSystem_.GetWinApp(), &context_);
}

void Engine::Finalize() {
    // 初期化と逆順で解放する。
    graphicsSystem_.Finalize();
    inputSystem_.Finalize();
    windowSystem_.Finalize();
}

void Engine::BeginFrame() {
    // 入力は Scene 更新前に最新状態へしておく。
    inputSystem_.Update();
}

bool Engine::IsRunning() {
    return !windowSystem_.ProcessMessage();
}