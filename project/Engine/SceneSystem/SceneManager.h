#pragma once

#include <memory>
#include <utility>

#include "IScene.h"

class EngineContext;

// Scene の所有と切り替えを担当するクラス。
// MyGame は具体的な Scene を直接メンバに持たず、SceneManager 経由で扱う。
class SceneManager {
public:
    void Initialize(EngineContext* context);
    void Finalize();

    void Update();
    void Draw();

    // TScene に指定した Scene へ切り替える。
    // 例: sceneManager_.ChangeScene<GameScene>();
    template <class TScene, class... Args>
    void ChangeScene(Args&&... args) {
        if (scene_) {
            scene_->Finalize();
        }

        scene_ = std::make_unique<TScene>(std::forward<Args>(args)...);
        scene_->Initialize(context_);
    }

private:
    // Scene に渡すエンジン機能の窓口。所有はしない。
    EngineContext* context_ = nullptr;

    // 現在実行中の Scene。
    std::unique_ptr<IScene> scene_;
};