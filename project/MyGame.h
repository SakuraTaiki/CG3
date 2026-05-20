#pragma once

#include <vector>
#include "WinApp.h"
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "Input.h"
#include "Object3d.h"
#include "Model.h"
#include "Sprite.h"
#include"ParticleManager.h"
#include "Sound.h"
#include "Skybox.h"

class MyGame
{
public:
    void Initialize();
    void Update();
    void Draw();
    void Finalize();
    bool IsRunning() { return !winApp->ProcessMessage(); }

private:
    // 基盤系
    WinApp* winApp = nullptr;
    DirectXCommon* dxCommon = nullptr;
    Input* input = nullptr;
    TextureManager* textureManager = nullptr;
    SpriteCommon* spriteCommon = nullptr;
    Object3dCommon* object3dCommon = nullptr;
    ParticleManager* particleManager = nullptr;

    // オブジェクト管理
    std::vector<Object3d*> objectList;
    std::vector<Model*> models; // モデル解放用
    Sprite* sprite = nullptr;
    Skybox* skybox = nullptr;

    Transform cameraTransform;

    // ヘルパー関数
    Object3d* CreateObject(Model* model, Vector3 pos);

    //サウンド管理
    Sound sound;
    //音声データ
    Sound::SoundData wavSoundData;
    Sound::SoundData mp4SoundData;
    Sound::SoundData mp3SoundData;

    //音量メンバ変数
    float wavVolume = 0.5f;
    float mp4Volume = 0.5f;
    float mp3Volume = 0.5f;
};

