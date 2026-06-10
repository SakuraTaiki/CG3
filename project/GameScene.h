#pragma once
#include <memory>
#include <vector>

#include "GameSystem.h"
#include "Object3d.h"
#include "Sprite.h"
#include "Skybox.h"
#include "Sound.h"

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

    void UpdateObjects();
    void UpdateSound();
    void UpdateImGui();

    void Draw3D();
    void Draw2D();

private:
    GameSystem* system_ = nullptr;

    std::vector<std::unique_ptr<Object3d>> objects_;

    std::unique_ptr<Sprite> sprite_;
    std::unique_ptr<Skybox> skybox_;

    Sound sound_;
    Sound::SoundData wavSoundData_{};
    Sound::SoundData mp4SoundData_{};
    Sound::SoundData mp3SoundData_{};

    float wavVolume_ = 0.5f;
    float mp4Volume_ = 0.5f;
    float mp3Volume_ = 0.5f;

    uint32_t environmentTexturehandle_ = 0;
    float environmentCoefficient_ = 0.05f;
};