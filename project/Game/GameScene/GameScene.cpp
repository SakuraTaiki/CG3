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

#ifndef USE_IMGUI
    // The submitted Release build is evaluated on the animation feature.
    // Start directly in the glTF animation view because no ImGui scene
    // selector is available in that configuration.
    drawMode_ = GameSceneDrawMode::Animation;
#endif

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

    UpdatePostEffectShortcuts();

    if (stageEditor_.IsGamePlayMode() && input->TriggerKey(DIK_SPACE)) {
        hitEffect_.Emit({ 0.0f, 3.0f, 0.0f });
    }

    soundController_.Update(context_->GetInput());
    UpdateTaskJsonHotReload();
    UpdateObjects();
    // Draw modes are exclusive tool contexts.  Animation mode must not
    // update the stage placement cursor or consume the same WASD input.
    stageEditor_.SetActive(drawMode_ == GameSceneDrawMode::NormalObj);
    stageEditor_.Update();

    cameraDebug_.Update(
        context_->GetCamera(),
        input
    );

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

void GameScene::UpdatePostEffectShortcuts() {
    Input* input = context_->GetInput();
    DirectXCommon* dxCommon = context_->GetDxCommon();
    if (!input || !dxCommon) {
        return;
    }

    auto& vignette = dxCommon->GetVignetteSettings();
    auto& smoothing = dxCommon->GetSmoothingSettings();
    auto& gaussian = dxCommon->GetGaussianSettings();
    auto& outline = dxCommon->GetOutlineSettings();
    auto& radialBlur = dxCommon->GetRadialBlurSettings();
    auto& dissolve = dxCommon->GetDissolveSettings();
    auto& random = dxCommon->GetRandomSettings();

    auto triggered = [input](BYTE mainKey, BYTE numpadKey) {
        return input->TriggerKey(mainKey) || input->TriggerKey(numpadKey);
    };

    enum class Effect {
        None,
        GrayScale,
        Vignette,
        Smoothing,
        Gaussian,
        Outline,
        RadialBlur,
        Dissolve,
        Random
    };

    Effect effect = Effect::None;
    bool enable = false;

    if (triggered(DIK_1, DIK_NUMPAD1)) {
        effect = Effect::GrayScale;
        enable = !dxCommon->GetGrayScale();
    } else if (triggered(DIK_2, DIK_NUMPAD2)) {
        effect = Effect::Vignette;
        enable = !vignette.enabled;
    } else if (triggered(DIK_3, DIK_NUMPAD3)) {
        effect = Effect::Smoothing;
        enable = !smoothing.enabled;
    } else if (triggered(DIK_4, DIK_NUMPAD4)) {
        effect = Effect::Gaussian;
        enable = !gaussian.enabled;
    } else if (triggered(DIK_5, DIK_NUMPAD5)) {
        effect = Effect::Outline;
        enable = !outline.enabled;
    } else if (triggered(DIK_6, DIK_NUMPAD6)) {
        effect = Effect::RadialBlur;
        enable = !radialBlur.enabled;
    } else if (triggered(DIK_7, DIK_NUMPAD7)) {
        effect = Effect::Dissolve;
        enable = !dissolve.enabled;
    } else if (triggered(DIK_8, DIK_NUMPAD8)) {
        effect = Effect::Random;
        enable = !random.enabled;
    }

    if (effect == Effect::None) {
        return;
    }

    dxCommon->SetGrayScale(false);
    vignette.enabled = false;
    smoothing.enabled = false;
    gaussian.enabled = false;
    outline.enabled = false;
    radialBlur.enabled = false;
    dissolve.enabled = false;
    random.enabled = false;

    if (!enable) {
        return;
    }

    switch (effect) {
    case Effect::GrayScale:
        dxCommon->SetGrayScale(true);
        break;
    case Effect::Vignette:
        vignette.enabled = true;
        break;
    case Effect::Smoothing:
        // Uniform 9x9 box blur: intentionally broad for the evaluation demo.
        smoothing.radius = 4;
        smoothing.strength = 1.0f;
        smoothing.enabled = true;
        break;
    case Effect::Gaussian:
        // Center-weighted 9x9 blur, visually distinct from the box filter.
        gaussian.radius = 4;
        gaussian.sigma = 1.0f;
        gaussian.strength = 1.0f;
        gaussian.enabled = true;
        break;
    case Effect::Outline:
        outline.enabled = true;
        break;
    case Effect::RadialBlur:
        radialBlur.enabled = true;
        break;
    case Effect::Dissolve:
        // The default threshold is zero and produces almost no visible change.
        dissolve.threshold = 0.5f;
        dissolve.edgeWidth = 0.08f;
        dissolve.edgeIntensity = 1.0f;
        dissolve.enabled = true;
        break;
    case Effect::Random:
        random.enabled = true;
        dxCommon->ResetRandomTime();
        break;
    default:
        break;
    }
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

    // The stage editor owns the editing viewport.  Do not mix the legacy
    // SceneObjectController scene (terrain/axis/Blender objects) into it:
    // placed stage items must be previewed against the same clean view that
    // will be used for the stage itself.  Runtime scene objects return when
    // entering GamePlay mode.
    if (
        drawMode_ == GameSceneDrawMode::NormalObj &&
        stageEditor_.IsGamePlayMode()
        ) {
        sceneObjects_.Draw();
    }

    if (drawMode_ == GameSceneDrawMode::NormalObj) {
        stageEditor_.Draw3D();
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


	dxCommon->DrawRenderTextureToSwapChain();
    
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
