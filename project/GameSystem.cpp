#include "GameSystem.h"

void GameSystem::Initialize() {
    winApp_ = std::make_unique<WinApp>();
    winApp_->Initialize();

    dxCommon_ = std::make_unique<DirectXCommon>();
    dxCommon_->Initialize(winApp_.get());

    input_ = std::make_unique<Input>();
    input_->Initialize(winApp_.get());

    srvManager_ = std::make_unique<SrvManager>();
    srvManager_->Initialize(dxCommon_.get());

    textureManager_ = std::make_unique<TextureManager>();
    textureManager_->Initialize(dxCommon_.get(), srvManager_.get());

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->SetTextureManager(textureManager_.get());
    spriteCommon_->Initialize(dxCommon_.get());

    object3dCommon_ = std::make_unique<Object3dCommon>();
    object3dCommon_->SetTextureManager(textureManager_.get());
    object3dCommon_->Initialize(dxCommon_.get());

    camera_ = std::make_unique<Camera>();

    camera_->Update();

    object3dCommon_->SetDefaultCamera(camera_.get());

    particleManager_ = std::make_unique<ParticleManager>();
    particleManager_->Initialize(dxCommon_.get(), textureManager_.get());

    imGuiManager_ = std::make_unique<ImGuiManager>();
    imGuiManager_->Initialize(dxCommon_.get(), srvManager_.get(), winApp_.get());
}

void GameSystem::Finalize() {
    if (imGuiManager_) {
        imGuiManager_->Finalize();
    }

    imGuiManager_.reset();
    particleManager_.reset();
    camera_.reset();
    object3dCommon_.reset();
    spriteCommon_.reset();
    textureManager_.reset();
    srvManager_.reset();
    input_.reset();
    dxCommon_.reset();
    winApp_.reset();
}

bool GameSystem::IsRunning() const {
    return !winApp_->ProcessMessage();
}