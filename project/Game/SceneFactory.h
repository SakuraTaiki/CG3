#pragma once

#include "AbstractSceneFactory.h"

// ゲームで使用する具体的な Scene の生成を担当する。
class SceneFactory final : public AbstractSceneFactory {
public:
    std::unique_ptr<IScene> CreateScene(const std::string& sceneName) override;
};
