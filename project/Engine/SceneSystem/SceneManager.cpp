#include "SceneManager.h"

void SceneManager::Initialize(EngineContext* context) {
    context_ = context;
}

void SceneManager::Finalize() {
    if (scene_) {
        scene_->Finalize();
        scene_.reset();
    }

    context_ = nullptr;
}

void SceneManager::Update() {
    if (scene_) {
        scene_->Update();
    }
}

void SceneManager::Draw() {
    if (scene_) {
        scene_->Draw();
    }
}