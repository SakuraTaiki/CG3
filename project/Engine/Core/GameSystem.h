#pragma once
#include <memory>

#include "WinApp.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "ImGuiManager.h"
#include "camera.h"

class GameSystem {
public:
    void Initialize();
    void Finalize();

    bool IsRunning() const;

    WinApp* GetWinApp() const { return winApp_.get(); }
    DirectXCommon* GetDxCommon() const { return dxCommon_.get(); }
    Input* GetInput() const { return input_.get(); }
    SrvManager* GetSrvManager() const { return srvManager_.get(); }
    TextureManager* GetTextureManager() const { return textureManager_.get(); }
    SpriteCommon* GetSpriteCommon() const { return spriteCommon_.get(); }
    Object3dCommon* GetObject3dCommon() const { return object3dCommon_.get(); }
    ParticleManager* GetParticleManager() const { return particleManager_.get(); }
    ImGuiManager* GetImGuiManager() const { return imGuiManager_.get(); }
    Camera* GetCamera() const { return camera_.get(); }

private:
    std::unique_ptr<WinApp> winApp_;
    std::unique_ptr<DirectXCommon> dxCommon_;
    std::unique_ptr<Input> input_;
    std::unique_ptr<SrvManager> srvManager_;
    std::unique_ptr<TextureManager> textureManager_;
    std::unique_ptr<SpriteCommon> spriteCommon_;
    std::unique_ptr<Object3dCommon> object3dCommon_;
    std::unique_ptr<ParticleManager> particleManager_;
    std::unique_ptr<ImGuiManager> imGuiManager_;
    std::unique_ptr<Camera> camera_;
};