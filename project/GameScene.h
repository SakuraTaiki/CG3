#pragma once

#include <memory>
#include <vector>

#include "IScene.h"
#include "EngineContext.h"

#include "Object3d.h"
#include "Sprite.h"
#include "Skybox.h"
#include "Sound.h"
#include "Ring.h"
#include "Cylinder.h"
#include "Primitive.h"
#include "Skelton.h"


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
    void InitializeSound();
    void InitializeRing();
    void InitializeCylinder();
    void InitializePrimitive();
    void InitializeSkeletonDebug();

    // 更新処理。
    void UpdateObjects();
    void UpdateSound();
    void UpdateImGui();
    void UpdateSkeletonDebug();

    // アニメーション確認用処理。
    void UpdateAnimationDebug();
    void DrawAnimationDebugImGui();
    void SyncAnimatedSkeletonToObject();

    // SPACE キーや ImGui ボタンから呼ばれるエフェクト発生処理。
    void EmitEffect(const Vector3& position);

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

    // アニメーション表示用の Object。
    std::unique_ptr<Object3d> animatedObject_;

    // Skeleton 確認用の表示 Object。
    std::vector<std::unique_ptr<Object3d>> skeletonDebugObjects_;
    std::vector<std::unique_ptr<Object3d>> skeletonBoneObjects_;

    Sound sound_;
    Sound::SoundData wavSoundData_{};
    Sound::SoundData mp4SoundData_{};
    Sound::SoundData mp3SoundData_{};

    float wavVolume_ = 0.5f;
    float mp4Volume_ = 0.5f;
    float mp3Volume_ = 0.5f;

    uint32_t environmentTexturehandle_ = 0;
    float environmentCoefficient_ = 0.05f;

    Vector3 ringPosition_ = { 0.0f, 2.0f, 0.0f };
    Vector3 ringScale_ = { 2.0f, 2.0f, 1.0f };
    Vector4 ringColor_ = { 1.0f, 1.0f, 1.0f, 0.7f };

    Vector3 hitEffectPosition_ = {
    0.0f,
    3.0f,
    0.0f
    };

    bool enableRing_ = true;
    bool enableCylinder_ = true;

    std::unique_ptr<Primitive> primitive_;
    bool enablePrimitive_ = true;

    Animation animatedCubeAnimation_;

    Skeleton animatedSkeleton_;
    float skeletonAnimationTime_ = 0.0f;

    bool showSkeletonDebug_ = true;

    bool animationPlaying_ = true;
    bool animationLoop_ = true;
    float animationSpeed_ = 1.0f;

    int selectedJointIndex_ = 0;
    bool autoPauseOnJointEdit_ = true;
    bool showJointDetail_ = true;
    bool showDebugSprite_ = false;
};