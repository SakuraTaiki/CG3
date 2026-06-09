#include "MyGame.h"

void MyGame::Initialize() {
    gameSystem_.Initialize();
    gameScene_.Initialize(&gameSystem_);
}

void MyGame::Update() {
    gameScene_.Update();
}

void MyGame::Draw() {
    gameScene_.Draw();
}

void MyGame::Finalize() {
    gameScene_.Finalize();
    gameSystem_.Finalize();
}

bool MyGame::IsRunning() {
    return gameSystem_.IsRunning();
}