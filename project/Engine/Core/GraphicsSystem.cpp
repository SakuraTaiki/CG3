#include "GraphicsSystem.h"

#include "EngineContext.h"
#include "WinApp.h"

void GraphicsSystem::Initialize(WinApp* winApp, EngineContext* context) {
    // DirectX 本体を最初に初期化する。
    dxCommon_ = std::make_unique<DirectXCommon>();
    dxCommon_->Initialize(winApp);
    context->SetDxCommon(dxCommon_.get());

    // SRV は Texture や ImGui などから使われる。
    srvManager_ = std::make_unique<SrvManager>();
    srvManager_->Initialize(dxCommon_.get());
    context->SetSrvManager(srvManager_.get());

    // TextureManager は DirectX と SRV に依存する。
    textureManager_ = std::make_unique<TextureManager>();
    textureManager_->Initialize(dxCommon_.get(), srvManager_.get());
    context->SetTextureManager(textureManager_.get());

    // Sprite 用の共通描画設定。
    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->SetTextureManager(textureManager_.get());
    spriteCommon_->Initialize(dxCommon_.get());
    context->SetSpriteCommon(spriteCommon_.get());

    // 3D Object 用の共通描画設定。
    object3dCommon_ = std::make_unique<Object3dCommon>();
    object3dCommon_->SetTextureManager(textureManager_.get());
    object3dCommon_->Initialize(dxCommon_.get());
    object3dCommon_->SetSrvManager(srvManager_.get());
    context->SetObject3dCommon(object3dCommon_.get());

    // デフォルトカメラを作成し、3D 描画側へ渡す。
    camera_ = std::make_unique<Camera>();

    camera_->SetAspectRatio(
        static_cast<float>(WinApp::kClientWidth) /
        static_cast<float>(WinApp::kClientHeight)
    );

    camera_->Update();

    object3dCommon_->SetDefaultCamera(camera_.get());
    context->SetCamera(camera_.get());

    // パーティクル描画。
    particleManager_ = std::make_unique<ParticleManager>();
    particleManager_->Initialize(dxCommon_.get(), textureManager_.get());
    context->SetParticleManager(particleManager_.get());

    gpuParticleManager_ = std::make_unique<GPUParticleManager>();
    gpuParticleManager_->Initialize(
        dxCommon_.get(),
        srvManager_.get(),
        textureManager_.get()
    );
    context->SetGPUParticleManager(gpuParticleManager_.get());

    // ImGui は DirectX / SRV / Window に依存するため最後の方で初期化する。
    imGuiManager_ = std::make_unique<ImGuiManager>();
    imGuiManager_->Initialize(dxCommon_.get(), srvManager_.get(), winApp);
    context->SetImGuiManager(imGuiManager_.get());
}

void GraphicsSystem::Finalize() {
    if (imGuiManager_) {
        imGuiManager_->Finalize();
    }

    // 依存関係があるため、初期化と逆順に解放する。
    imGuiManager_.reset();
    gpuParticleManager_.reset();
    particleManager_.reset();
    camera_.reset();
    object3dCommon_.reset();
    spriteCommon_.reset();
    textureManager_.reset();
    srvManager_.reset();
    dxCommon_.reset();
}
