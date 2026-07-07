#include "GameScene.h"

#include "ModelManager.h"

#include "Input.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "GPUParticleManager.h"
#include "ImGuiManager.h"
#include "camera.h"
#include "WinApp.h"

#include <algorithm>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
void GameScene::Initialize(EngineContext* context) {
    context_ = context;

    ModelManager::Initialize(context_->GetObject3dCommon());

    InitializeModels();
    InitializeSprite();

    environment_.Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager()
    );

    Object3dCommon* object3dCommon =
        context_->GetObject3dCommon();

    sceneObjects_.Initialize(
        object3dCommon,
        environment_.GetEnvironmentTextureHandle(),
        environment_.GetEnvironmentCoefficient()
    );

    animationDebug_.Initialize(
        object3dCommon,
        environment_.GetEnvironmentTextureHandle(),
        environment_.GetEnvironmentCoefficient()
    );

    InitializeRing();
    InitializeCylinder();
    InitializePrimitive();



    hitEffect_.Initialize(
        primitive_.get(),
        ring_.get(),
        cylinder_.get(),
        context_->GetParticleManager(),
        context_->GetGPUParticleManager()
    );

    hitEffect_.ApplyFirePreset();
    hitEffect_.RefreshPresetList();

    soundController_.Initialize();
}

void GameScene::Finalize() {
    sceneObjects_.Finalize();

    animationDebug_.Finalize();
    environment_.Finalize();
    sprite_.reset();

    soundController_.Finalize();
    ModelManager::Finalize();

    context_ = nullptr;
}

void GameScene::InitializeModels() {
    ModelManager::Load("Resources/terrain", "terrain.obj");
    ModelManager::Load("axis.obj");

    ModelManager::Load("Resources/human", "walk.gltf");
}




void GameScene::InitializeSprite() {
    uint32_t texHandle =
        context_->GetTextureManager()->LoadTexture("Resources/white.png");

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(context_->GetSpriteCommon(), texHandle);
}


void GameScene::InitializeRing() {
    ring_ = std::make_unique<Ring>();
    ring_->Initialize(context_->GetDxCommon(), context_->GetTextureManager());
}

void GameScene::InitializeCylinder()
{
    cylinder_ = std::make_unique<Cylinder>();
    cylinder_->Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager()
    );
}

void GameScene::InitializePrimitive()
{
    primitive_ = std::make_unique<Primitive>();

    primitive_->Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager()
    );

}


void GameScene::Update() {
    Input* input = context_->GetInput();

    if (input->TriggerKey(DIK_SPACE)) {
        hitEffect_.Emit({ 0.0f, 3.0f, 0.0f });
    }

    soundController_.Update(context_->GetInput());
    UpdateObjects();


    WinApp* winApp =
        context_->GetWinApp();

    if (
        winApp &&
        winApp->GetHeight() > 0
        ) {
        context_->GetCamera()->SetAspectRatio(
            static_cast<float>(
                winApp->GetWidth()
                ) /
            static_cast<float>(
                winApp->GetHeight()
                )
        );
    }

    context_->GetCamera()->Update();

    const Matrix4x4& view = context_->GetCamera()->GetViewMatrix();
    const Matrix4x4& projection = context_->GetCamera()->GetProjectionMatrix();

    environment_.Update(
        view,
        projection
    );

    if (sprite_) {
        sprite_->Update();
    }

    if (ring_) {
        ring_->SetIsActive(hitEffect_.EnableRing());
        ring_->Update(view, projection);
    }

    if (cylinder_) {
        cylinder_->SetIsActive(hitEffect_.EnableCylinder());
        cylinder_->Update(view, projection);
    }

    if (primitive_) {
        primitive_->SetIsActive(
            hitEffect_.EnablePrimitive()
        );

        primitive_->Update(
            view,
            projection
        );
    }

    context_->GetParticleManager()->Update(view, projection);
    context_->GetGPUParticleManager()->Update(view, projection);

    sceneDebugPanel_.Draw(
        context_,
        drawMode_,
        hitEffect_,
        animationDebug_,
        soundController_,
        cameraDebug_,
        environment_,
        sceneObjects_,
        ring_.get(),
        cylinder_.get(),
        primitive_.get()
    );
}

void GameScene::UpdateObjects() {
    if (drawMode_ == GameSceneDrawMode::NormalObj) {
        sceneObjects_.Update();
    }

    if (drawMode_ == GameSceneDrawMode::Animation) {
        animationDebug_.Update();
    }
}



void GameScene::Draw() {
    Draw3D();
    Draw2D();
}

void GameScene::Draw3D() {
    DirectXCommon* dxCommon = context_->GetDxCommon();

    dxCommon->PreDrawForRenderTexture();

    context_->GetSrvManager()->PreDraw();

    context_->GetObject3dCommon()->PreDraw();

    if (drawMode_ == GameSceneDrawMode::NormalObj) {
        sceneObjects_.Draw();
    }

    if (drawMode_ == GameSceneDrawMode::Animation) {
        animationDebug_.Draw();
    }

    environment_.Draw();

    if (ring_) {
        ring_->Draw();
    }

    if (cylinder_) {
        cylinder_->Draw();
    }

    if (primitive_) {
        primitive_->Draw();
    }

    context_->GetParticleManager()->Draw();
    context_->GetGPUParticleManager()->Draw();
}



void GameScene::Draw2D()
{
    DirectXCommon* dxCommon =
        context_->GetDxCommon();

    dxCommon->PreDraw();

    // Near/FarだけはCameraと毎フレーム同期する
    Camera* camera =
        context_->GetCamera();

    if (camera) {
        auto& outline =
            dxCommon->GetOutlineSettings();

        outline.nearClip =
            camera->GetNearClip();

        outline.farClip =
            camera->GetFarClip();
    }

    // enabled、色、強度、太さなどは
    // ImGuiで変更した値をそのまま使用する
    dxCommon->PrepareRenderTextureForImgui();

    context_->GetSrvManager()->PreDraw();
    context_->GetSpriteCommon()->PreDraw();

    if (sprite_ && showDebugSprite_) {
        sprite_->Draw();
    }

    context_->GetImGuiManager()->Draw();

    dxCommon->PostDraw();
}