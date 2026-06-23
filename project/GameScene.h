#pragma once

#include <memory>
#include <vector>

#include "IScene.h"
#include "EngineContext.h"

#include "Object3d.h"
#include "Sprite.h"
#include "Skybox.h"
#include "Ring.h"
#include "Cylinder.h"
#include "Primitive.h"
#include "HitEffectController.h"
#include "AnimationDebugController.h"
#include "SoundController.h"


// 実際のゲーム・デモ内容を持つ Scene。
// EngineContext 経由で Input / Graphics / Camera などを使う。
class GameScene : public IScene {
public:
    void Initialize(EngineContext* context) override;
    void Finalize() override;

    void Update() override;
    void Draw() override;


private:
    // 初期化処理を役割ごとに分ける。
    void InitializeModels();
    void InitializeSprite();
    void InitializeSkybox();
    void InitializeObjects();
    void InitializeRing();
    void InitializeCylinder();
    void InitializePrimitive();
   

    // 更新処理。
    void UpdateObjects();
    void UpdateImGui();

    // 描画処理。
    void Draw3D();
    void Draw2D();

private:
    enum class DrawMode {
        NormalObj,
        Animation
    };

    DrawMode drawMode_ = DrawMode::NormalObj;

    // 非所有。実体は Engine が持つ。
    EngineContext* context_ = nullptr;

    std::vector<std::unique_ptr<Object3d>> objects_;

    std::unique_ptr<Sprite> sprite_;
    std::unique_ptr<Skybox> skybox_;
    std::unique_ptr<Ring> ring_;
    std::unique_ptr<Cylinder> cylinder_;

    uint32_t environmentTexturehandle_ = 0;
    float environmentCoefficient_ = 0.05f;

    Vector3 ringPosition_ = { 0.0f, 2.0f, 0.0f };
    Vector3 ringScale_ = { 2.0f, 2.0f, 1.0f };
    Vector4 ringColor_ = { 1.0f, 1.0f, 1.0f, 0.7f };

    std::unique_ptr<Primitive> primitive_;

   
    bool showDebugSprite_ = false;
    bool enableSkybox_ = true;

    HitEffectController hitEffect_;
    AnimationDebugController animationDebug_;
    SoundController soundController_;
};