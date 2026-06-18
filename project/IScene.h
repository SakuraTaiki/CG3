#pragma once

class EngineContext;

// Scene 共通インターフェース。
// SceneManager はこの型で Scene を保持する。
class IScene {
public:
    virtual ~IScene() = default;

    virtual void Initialize(EngineContext* context) = 0;
    virtual void Finalize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
};