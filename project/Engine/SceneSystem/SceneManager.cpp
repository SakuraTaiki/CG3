#include "SceneManager.h"

#include <utility>

void SceneManager::Initialize(EngineContext* context, AbstractSceneFactory* sceneFactory) {
    context_ = context;
    sceneFactory_ = sceneFactory;
}

void SceneManager::Finalize() {
    if (scene_) {
        scene_->Finalize();
        scene_.reset();
    }

    sceneFactory_ = nullptr;
    context_ = nullptr;
}

bool SceneManager::ChangeScene(const std::string& sceneName) {
    if (!context_ || !sceneFactory_) {
        return false;
    }

    std::unique_ptr<IScene> nextScene = sceneFactory_->CreateScene(sceneName);
    if (!nextScene) {
        return false;
    }

    if (scene_) {
        scene_->Finalize();
    }

    scene_ = std::move(nextScene);
    scene_->Initialize(context_);
    return true;
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
