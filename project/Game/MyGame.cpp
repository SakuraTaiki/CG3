#include "MyGame.h"

void MyGame::Initialize() {
    // エンジン基盤を先に初期化する。
    engine_.Initialize();

    // Scene は EngineContext 経由で各システムへアクセスする。
    sceneManager_.Initialize(engine_.GetContext(), &sceneFactory_);
    sceneManager_.ChangeScene("GAME");
}

void MyGame::Update() {
    // 入力など、Scene より前に必要な共通処理。
    engine_.BeginFrame();

    sceneManager_.Update();
}

void MyGame::Draw() {
    sceneManager_.Draw();
}

void MyGame::Finalize() {
    // Scene は Engine の機能を参照しているので先に破棄する。
    sceneManager_.Finalize();

    engine_.Finalize();
}

bool MyGame::IsRunning() {
    return engine_.IsRunning();
}
