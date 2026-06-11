#pragma once
#include <memory>
#include <vector>

#include "GameSystem.h"
#include "Object3d.h"
#include "Sprite.h"
#include "Skybox.h"
#include "Sound.h"
#include "Ring.h"
#include "Cylinder.h"
#include "Skelton.h"

class GameScene {
public:
    void Initialize(GameSystem* system);
    void Finalize();

    void Update();
    void Draw();

private:
    void InitializeModels();
    void InitializeSprite();
    void InitializeSkybox();
    void InitializeObjects();
    void InitializeSound();
    void InitializeRing();
    void InitializeCylinder();
    void InitializeSkeletonDebug();

    void UpdateObjects();
    void UpdateSound();
    void UpdateImGui();
    void UpdateSkeletonDebug();

    void Draw3D();
    void Draw2D();

private:

    enum class DrawMode {
        NormalObj,
        Animation
    };

    DrawMode drawMode_ = DrawMode::NormalObj;

    GameSystem* system_ = nullptr;

    std::vector<std::unique_ptr<Object3d>> objects_;

    std::unique_ptr<Sprite> sprite_;
    std::unique_ptr<Skybox> skybox_;
    std::unique_ptr<Ring>   ring_;
    std::unique_ptr<Cylinder> cylinder_;
    std::unique_ptr<Object3d> animatedObject_;
    std::vector<std::unique_ptr<Object3d>> skeletonDebugObjects_;
    

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

    bool enableRing_ = true;
    bool enableCylinder_ = true;

    Animation animatedCubeAnimation_;

    Skeleton animatedSkeleton_;
    float skeletonAnimationTime_ = 0.0f;

    bool showSkeletonDebug_ = true;
};