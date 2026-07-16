#include "SceneFactory.h"

#include "GameScene.h"

std::unique_ptr<IScene> SceneFactory::CreateScene(const std::string& sceneName) {
    if (sceneName == "GAME") {
        return std::make_unique<GameScene>();
    }

    return nullptr;
}
