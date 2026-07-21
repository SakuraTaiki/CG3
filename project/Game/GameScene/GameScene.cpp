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
#include <filesystem>

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

    // Blender export is authoritative whenever it exists. A missing model
    // referenced by Task.json must not make us replace the whole scene with
    // LevelScene.json.
    const bool hasBlenderTask =
        std::filesystem::exists("Resources/Task.json");
    bool levelLoaded = false;
    if (hasBlenderTask) {
        levelLoaded = sceneObjects_.LoadLevelSceneFromJson(
            "Resources/Task.json"
        );
    } else {
        levelLoaded = sceneObjects_.LoadLevelSceneFromJson(
            "Resources/LevelScene.json"
        );
    }

    sceneObjects_.ShowTerrain() = false;
    sceneObjects_.ShowAxis() = false;

    if (!levelLoaded) {
        OutputDebugStringA(
            hasBlenderTask
                ? "Task.json was loaded with missing or invalid assets\n"
                : "Failed to load Resources/LevelScene.json\n"
        );
    }

    InitializeTaskJsonHotReload();

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
    stageEditor_.Initialize(
        object3dCommon,
        environment_.GetEnvironmentTextureHandle(),
        environment_.GetEnvironmentCoefficient()
    );
}

void GameScene::Finalize() {
    stageEditor_.Finalize();
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

    if (stageEditor_.IsGamePlayMode() && input->TriggerKey(DIK_SPACE)) {
        hitEffect_.Emit({ 0.0f, 3.0f, 0.0f });
    }

    soundController_.Update(context_->GetInput());
    UpdateTaskJsonHotReload();
    UpdateObjects();
    stageEditor_.Update();


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
        stageEditor_,
        ring_.get(),
        cylinder_.get(),
        primitive_.get()
    );
}

void GameScene::InitializeTaskJsonHotReload() {
    constexpr const char* kTaskJsonPath = "Resources/Task.json";
    std::error_code error;

    if (!std::filesystem::exists(kTaskJsonPath, error) || error) {
        taskJsonWatchInitialized_ = false;
        taskJsonReloadPending_ = false;
        return;
    }

    taskJsonObservedWriteTime_ =
        std::filesystem::last_write_time(kTaskJsonPath, error);
    taskJsonWatchInitialized_ = !error;
    taskJsonReloadPending_ = false;
    taskJsonReloadAttempts_ = 0;
}

void GameScene::UpdateTaskJsonHotReload() {
    constexpr const char* kTaskJsonPath = "Resources/Task.json";
    constexpr auto kReloadDebounce = std::chrono::milliseconds(250);
    constexpr int kMaxReloadAttempts = 10;

    std::error_code error;
    if (!std::filesystem::exists(kTaskJsonPath, error) || error) {
        taskJsonWatchInitialized_ = false;
        taskJsonReloadPending_ = false;
        return;
    }

    const auto currentWriteTime =
        std::filesystem::last_write_time(kTaskJsonPath, error);
    if (error) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!taskJsonWatchInitialized_) {
        taskJsonObservedWriteTime_ = currentWriteTime;
        taskJsonWatchInitialized_ = true;
        taskJsonReloadPending_ = true;
        taskJsonReloadAttempts_ = 0;
        taskJsonChangeDetectedAt_ = now;
        return;
    }

    if (currentWriteTime != taskJsonObservedWriteTime_) {
        taskJsonObservedWriteTime_ = currentWriteTime;
        taskJsonReloadPending_ = true;
        taskJsonReloadAttempts_ = 0;
        taskJsonChangeDetectedAt_ = now;
        return;
    }

    if (!taskJsonReloadPending_ ||
        now - taskJsonChangeDetectedAt_ < kReloadDebounce) {
        return;
    }

    const bool loaded =
        sceneObjects_.LoadLevelSceneFromJson(kTaskJsonPath);
    if (loaded) {
        taskJsonReloadPending_ = false;
        taskJsonReloadAttempts_ = 0;
        OutputDebugStringA("Hot reloaded Resources/Task.json\n");
        return;
    }

    ++taskJsonReloadAttempts_;
    if (taskJsonReloadAttempts_ >= kMaxReloadAttempts) {
        taskJsonReloadPending_ = false;
        OutputDebugStringA("Task.json hot reload failed after retries\n");
    } else {
        taskJsonChangeDetectedAt_ = now;
    }
}

void GameScene::UpdateObjects() {
    if (drawMode_ == GameSceneDrawMode::NormalObj) {
        sceneObjects_.Update();
    }

    if (drawMode_ == GameSceneDrawMode::Animation) {
        animationDebug_.Update(context_->GetInput());
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

    stageEditor_.Draw3D();

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

    
    dxCommon->PrepareRenderTextureForImgui();

    if (context_->GetImGuiManager()) {
        context_->GetImGuiManager()->UpdateGameViewTexture();
    }

    context_->GetSrvManager()->PreDraw();
    context_->GetSpriteCommon()->PreDraw();

    if (sprite_ && showDebugSprite_) {
        sprite_->Draw();
    }

    context_->GetImGuiManager()->Draw();

    dxCommon->PostDraw();
}
