#pragma once

#include <memory>
#include <string>

#include "IScene.h"

// Scene の生成を SceneManager から分離するための抽象ファクトリー。
class AbstractSceneFactory {
public:
    virtual ~AbstractSceneFactory() = default;

    virtual std::unique_ptr<IScene> CreateScene(const std::string& sceneName) = 0;
};
