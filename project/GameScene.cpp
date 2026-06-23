#include "GameScene.h"

#include "ModelManager.h"

#include "Input.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
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
    InitializeSkybox();
    InitializeObjects();
    InitializeRing();
    InitializeCylinder();
    InitializePrimitive();

    hitEffect_.Initialize(
        primitive_.get(),
        ring_.get(),
        cylinder_.get(),
        context_->GetParticleManager()
    );

    hitEffect_.ApplyFirePreset();
    hitEffect_.RefreshPresetList();

    soundController_.Initialize();
}

void GameScene::Finalize() {
    objects_.clear();
    
    animationDebug_.Finalize();

    skybox_.reset();
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

void GameScene::InitializeObjects() {
    Object3dCommon* object3dCommon = context_->GetObject3dCommon();

    Model* modelPlane = ModelManager::Load("Resources/terrain", "terrain.obj");
    Model* modelAxis = ModelManager::Load("axis.obj");

    {
        std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
        object->Initialize(object3dCommon);
        object->SetModel(modelPlane);
        object->SetPosition({ 0.0f, 1.0f, 0.0f });
        object->SetRotation({ 0.0f, 0.0f, 0.0f });
        object->SetScale({ 0.5f, 0.5f, 0.5f });

        object->SetEnvironmentTexture(environmentTexturehandle_);
        object->SetEnvironmentCoefficient(environmentCoefficient_);

        objects_.push_back(std::move(object));
    }

    {
        std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
        object->Initialize(object3dCommon);
        object->SetModel(modelAxis);
        object->SetPosition({ 2.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });

        object->SetEnvironmentTexture(environmentTexturehandle_);
        object->SetEnvironmentCoefficient(environmentCoefficient_);

        objects_.push_back(std::move(object));
    }

    {
        std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
        object->Initialize(object3dCommon);
        object->SetModel(modelAxis);
        object->SetPosition({ -2.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });

        object->SetEnvironmentTexture(environmentTexturehandle_);
        object->SetEnvironmentCoefficient(environmentCoefficient_);

        objects_.push_back(std::move(object));
    }

    animationDebug_.Initialize(
        object3dCommon,
        environmentTexturehandle_,
        environmentCoefficient_
    );
}


void GameScene::InitializeSprite() {
    uint32_t texHandle =
        context_->GetTextureManager()->LoadTexture("Resources/white.png");

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(context_->GetSpriteCommon(), texHandle);
}

void GameScene::InitializeSkybox() {
    skybox_ = std::make_unique<Skybox>();

    skybox_->Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager(),
        "Resources/skybox/rostock_laage_airport_4k.dds"
    );

    skybox_->SetScale({ 100.0f, 100.0f, 100.0f });

    environmentTexturehandle_ =
        context_->GetTextureManager()->LoadTexture(
            "Resources/skybox/rostock_laage_airport_4k.dds"
        );
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

    if (enableSkybox_ && skybox_) {
        skybox_->SetCamera(
            view,
            projection
        );

        skybox_->Update();
    }

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

    UpdateImGui();
}

void GameScene::UpdateObjects() {
    if (drawMode_ == DrawMode::NormalObj) {
        for (auto& object : objects_) {
            object->Update();
        }
    }

    if (drawMode_ == DrawMode::Animation) {
        animationDebug_.Update();
    }
}


void GameScene::UpdateImGui() {
#ifdef USE_IMGUI

    context_->GetImGuiManager()->Begin();

    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin(
        "CG2 Effect Debug Panel",
        nullptr,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

    ImGui::Text("Scene Control");
    ImGui::Separator();

    if (ImGui::Button("Emit Effect SPACE", ImVec2(180.0f, 32.0f))) {
        hitEffect_.Emit({ 0.0f, 3.0f, 0.0f });
    }

    ImGui::SameLine();
    ImGui::TextDisabled("SPACE key also emits");

    ImGui::Spacing();

    ImGui::Text("Draw Mode");
    ImGui::Separator();

    int currentMode = static_cast<int>(drawMode_);

    if (ImGui::RadioButton("Normal OBJ / Terrain", currentMode == 0)) {
        drawMode_ = DrawMode::NormalObj;
    }

    if (ImGui::RadioButton("Animation glTF", currentMode == 1)) {
        drawMode_ = DrawMode::Animation;
    }

    ImGui::Spacing();

    ImGui::Text("Skeleton");
    ImGui::BulletText("Joint Count : %d", animationDebug_.GetJointCount());
    ImGui::BulletText("Animation Time : %.2f", animationDebug_.GetAnimationTime());
    ImGui::Spacing();

    ImGui::Checkbox(
        "Show Skeleton Debug",
        &animationDebug_.ShowSkeletonDebug()
    );

    

    if (ImGui::BeginTabBar("MainTabs")) {

        if (ImGui::BeginTabItem("Animation")) {
            animationDebug_.DrawImGui();
            ImGui::EndTabItem();
        }

        // ==================================================
        // Effect
        // ==================================================
        if (ImGui::BeginTabItem("Effect")) {
            ImGui::Checkbox(
                "Enable Primitive",
                &hitEffect_.EnablePrimitive()
            );

            ImGui::Checkbox(
                "Enable Ring",
                &hitEffect_.EnableRing()
            );

            ImGui::Checkbox(
                "Enable Cylinder",
                &hitEffect_.EnableCylinder()
            );

            ImGui::SeparatorText("Hit Effect Type");

            if (ImGui::RadioButton(
                "Fire",
                hitEffect_.GetType() == HitEffectController::Type::Fire
            )) {
                hitEffect_.GetType() =
                    HitEffectController::Type::Fire;
            }

            ImGui::SameLine();

            if (ImGui::RadioButton(
                "Sakura",
                hitEffect_.GetType() == HitEffectController::Type::Sakura
            )) {
                hitEffect_.GetType() =
                    HitEffectController::Type::Sakura;
            }

            ImGui::SliderFloat(
                "Hit Effect Size",
                &hitEffect_.GetSize(),
                0.1f,
                5.0f,
                "%.2f"
            );

            if (
                hitEffect_.GetType() ==
                HitEffectController::Type::Sakura
                ) {
                ParticleManager::SakuraSettings& settings =
                    context_
                    ->GetParticleManager()
                    ->GetSakuraSettings();

                ImGui::SeparatorText("Sakura Settings");

                ImGui::ColorEdit4(
                    "Sakura Main Color",
                    &settings.color.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::ColorEdit4(
                    "Sakura Sub Color",
                    &settings.subColor.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::DragInt(
                    "Petal Count",
                    &settings.petalCount,
                    1.0f,
                    1,
                    256
                );

                ImGui::DragFloatRange2(
                    "Petal Size",
                    &settings.minSize,
                    &settings.maxSize,
                    0.005f,
                    0.01f,
                    2.0f
                );

                ImGui::DragFloat(
                    "Spawn Radius",
                    &settings.spawnRadius,
                    0.01f,
                    0.0f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Spread Speed",
                    &settings.spreadSpeed,
                    0.001f,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloat(
                    "Upward Speed",
                    &settings.upwardSpeed,
                    0.001f,
                    -1.0f,
                    1.0f
                );

                ImGui::DragFloatRange2(
                    "Petal Life Time",
                    &settings.minLifeTime,
                    &settings.maxLifeTime,
                    0.01f,
                    0.05f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Petal Gravity",
                    &settings.gravity,
                    0.0001f,
                    -0.1f,
                    0.1f,
                    "%.4f"
                );

                ImGui::DragFloat(
                    "Petal Rotation",
                    &settings.rotationSpeed,
                    0.005f,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloat(
                    "Flash Size",
                    &settings.flashSize,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Flash Life Time",
                    &settings.flashLifeTime,
                    0.01f,
                    0.05f,
                    5.0f
                );

                if (ImGui::Button(
                    "Reset Sakura Settings"
                )) {
                    settings =
                        ParticleManager::SakuraSettings{};
                }
            }


            ImGui::SeparatorText("Emit");

            ImGui::DragFloat3(
                "Effect Position",
                &hitEffect_.GetPosition().x,
                0.05f,
                -20.0f,
                20.0f
            );

            ImGui::SeparatorText("Hit Effect Size");

            ImGui::SliderFloat(
                "Effect Size",
                &hitEffect_.GetSize(),
                0.1f,
                5.0f,
                "%.2f"
            );

            if (ImGui::Button("Reset Effect Size")) {
                hitEffect_.GetSize() = 1.0f;
            }

            ImGui::SameLine();

            ImGui::TextDisabled(
                "Current : %.2f x",
                hitEffect_.GetSize()
            );


            ImGui::SeparatorText("Effect Presets");

            ImGui::InputText(
                "Preset Name",
                hitEffect_.GetPresetNameBuffer().data(),
                hitEffect_.GetPresetNameBuffer().size()
            );

            if (ImGui::Button(
                "Save Current Preset",
                ImVec2(180.0f, 30.0f)
            )) {
                hitEffect_.SavePreset(
                    hitEffect_.GetPresetNameBuffer().data()
                );
            }

            ImGui::SameLine();

            if (ImGui::Button(
                "Refresh Presets",
                ImVec2(160.0f, 30.0f)
            )) {
                hitEffect_.RefreshPresetList();
            }

            if (!hitEffect_.GetPresetNames().empty()) {
                hitEffect_.GetSelectedPreset() =
                    std::clamp(
                        hitEffect_.GetSelectedPreset(),
                        0,
                        static_cast<int>(
                            hitEffect_.GetPresetNames().size()
                            ) - 1
                    );

                const char* previewName =
                    hitEffect_.GetPresetNames()[
                        hitEffect_.GetSelectedPreset()
                    ].c_str();

                if (ImGui::BeginCombo(
                    "Saved Presets",
                    previewName
                )) {
                    for (
                        int index = 0;
                        index <
                        static_cast<int>(
                            hitEffect_.GetPresetNames().size()
                            );
                            ++index
                        ) {
                        const bool selected =
                            hitEffect_.GetSelectedPreset() ==
                            index;

                        if (ImGui::Selectable(
                            hitEffect_.GetPresetNames()[index]
                            .c_str(),
                            selected
                        )) {
                            hitEffect_.GetSelectedPreset() =
                                index;
                        }

                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::Button(
                    "Load Selected",
                    ImVec2(180.0f, 30.0f)
                )) {
                    hitEffect_.LoadPreset(
                        hitEffect_.GetPresetNames()[
                            hitEffect_.GetSelectedPreset()
                        ]
                    );
                }

                ImGui::SameLine();

                if (ImGui::Button(
                    "Load And Preview",
                    ImVec2(180.0f, 30.0f)
                )) {
                    if (hitEffect_.LoadPreset(
                        hitEffect_.GetPresetNames()[
                            hitEffect_.GetSelectedPreset()
                        ]
                    )) {
                        hitEffect_.Emit(
                            hitEffect_.GetPosition()
                        );
                    }
                }
            } else {
                ImGui::TextDisabled(
                    "No saved presets"
                );
            }

            if (!hitEffect_.GetMessage().empty()) {
                ImGui::TextDisabled(
                    "%s",
                    hitEffect_.GetMessage().c_str()
                );
            }

            ImGui::SeparatorText("Preview");

            if (ImGui::Button(
                "Preview Current",
                ImVec2(180.0f, 32.0f)
            )) {
                hitEffect_.Emit(
                    hitEffect_.GetPosition()
                );
            }

            if (ImGui::Button(
                "Apply Fire Preset",
                ImVec2(180.0f, 30.0f)
            )) {
                hitEffect_.GetType() =
                    HitEffectController::Type::Fire;

                hitEffect_.ApplyFirePreset();
            }

            ImGui::SameLine();

            if (ImGui::Button(
                "Apply Fire And Preview",
                ImVec2(180.0f, 30.0f)
            )) {
                hitEffect_.GetType() =
                    HitEffectController::Type::Fire;

                hitEffect_.ApplyFirePreset();

                hitEffect_.Emit(
                    hitEffect_.GetPosition()
                );
            }

            ImGui::SameLine();

            if (ImGui::Button(
                "Preview Fire",
                ImVec2(180.0f, 30.0f)
            )) {
                hitEffect_.Emit(hitEffect_.GetPosition());
            }

            if (ring_) {
                Ring::Settings& settings =
                    ring_->GetSettings();

                ImGui::SeparatorText("Ring");

                ImGui::ColorEdit4(
                    "Ring Start Color",
                    &settings.color.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::ColorEdit4(
                    "Ring End Color",
                    &settings.endColor.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::DragFloat(
                    "Ring Glow Intensity",
                    &settings.intensity,
                    0.01f,
                    0.0f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Ring Start Scale",
                    &settings.startScale,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Ring End Scale",
                    &settings.endScale,
                    0.01f,
                    0.01f,
                    20.0f
                );

                ImGui::SliderFloat(
                    "Ring Start Thickness",
                    &settings.thickness,
                    0.001f,
                    0.5f
                );

                ImGui::SliderFloat(
                    "Ring End Thickness",
                    &settings.endThickness,
                    0.001f,
                    0.5f
                );

                ImGui::DragFloat(
                    "Ring Life Time",
                    &settings.lifeTime,
                    0.01f,
                    0.05f,
                    5.0f
                );

                ImGui::SliderFloat(
                    "Ring Fade In Ratio",
                    &settings.fadeInRatio,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloat(
                    "Ring Ease Power",
                    &settings.easePower,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Ring Rotation Speed",
                    &settings.rotationSpeed,
                    0.01f,
                    -20.0f,
                    20.0f
                );

                ImGui::DragFloat(
                    "Ring Edge Softness",
                    &settings.edgeSoftness,
                    0.001f,
                    0.001f,
                    0.25f
                );

                ImGui::DragFloat(
                    "Ring Glow Strength",
                    &settings.glowStrength,
                    0.01f,
                    0.0f,
                    5.0f
                );

                ImGui::DragFloat(
                    "Ring Distortion",
                    &settings.distortionStrength,
                    0.001f,
                    0.0f,
                    0.5f
                );

                ImGui::DragFloat(
                    "Ring Distortion Frequency",
                    &settings.distortionFrequency,
                    0.1f,
                    1.0f,
                    32.0f
                );

                ImGui::DragFloat(
                    "Ring Distortion Speed",
                    &settings.distortionSpeed,
                    0.01f,
                    -10.0f,
                    10.0f
                );

                if (ImGui::Button("Reset Ring")) {
                    settings =
                        Ring::Settings{};
                }
            }


            if (primitive_) {
                Primitive::Settings& settings =
                    primitive_->GetSettings();

                ImGui::SeparatorText("Primitive");

                ImGui::ColorEdit4(
                    "Primitive Start Color",
                    &settings.color.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::ColorEdit4(
                    "Primitive End Color",
                    &settings.endColor.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::DragInt(
                    "Primitive Count",
                    &settings.count,
                    1.0f,
                    1,
                    128
                );

                ImGui::DragFloat(
                    "Primitive Glow",
                    &settings.intensity,
                    0.01f,
                    0.0f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Primitive Width",
                    &settings.width,
                    0.001f,
                    0.001f,
                    2.0f
                );

                ImGui::SliderFloat(
                    "Primitive Width Random",
                    &settings.widthRandomness,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloatRange2(
                    "Primitive Length",
                    &settings.minLength,
                    &settings.maxLength,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::DragFloatRange2(
                    "Primitive Life Time",
                    &settings.minLifeTime,
                    &settings.maxLifeTime,
                    0.01f,
                    0.01f,
                    5.0f
                );

                ImGui::DragFloat(
                    "Primitive Move Speed",
                    &settings.moveSpeed,
                    0.01f,
                    0.0f,
                    20.0f
                );

                ImGui::SliderFloat(
                    "Primitive Speed Random",
                    &settings.moveSpeedRandomness,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloat(
                    "Primitive Rotation Speed",
                    &settings.rotationSpeed,
                    0.01f,
                    -20.0f,
                    20.0f
                );

                ImGui::DragFloat(
                    "Primitive Rotation Random",
                    &settings.rotationSpeedRandomness,
                    0.01f,
                    0.0f,
                    20.0f
                );

                ImGui::SliderAngle(
                    "Primitive Direction",
                    &settings.directionAngle,
                    -180.0f,
                    180.0f
                );

                ImGui::SliderAngle(
                    "Primitive Direction Spread",
                    &settings.directionSpread,
                    0.0f,
                    180.0f
                );

                ImGui::DragFloat(
                    "Primitive Spawn Radius",
                    &settings.spawnRadius,
                    0.01f,
                    0.0f,
                    10.0f
                );

                ImGui::DragFloat3(
                    "Primitive Acceleration",
                    &settings.acceleration.x,
                    0.01f,
                    -20.0f,
                    20.0f
                );

                ImGui::DragFloat(
                    "Primitive End Width Scale",
                    &settings.endWidthScale,
                    0.01f,
                    0.0f,
                    5.0f
                );

                ImGui::DragFloat(
                    "Primitive End Length Scale",
                    &settings.endLengthScale,
                    0.01f,
                    0.0f,
                    5.0f
                );

                ImGui::DragFloat(
                    "Primitive Scale Ease",
                    &settings.scaleEasePower,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::SliderFloat(
                    "Primitive Fade In",
                    &settings.fadeInRatio,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloat(
                    "Primitive Fade Power",
                    &settings.fadePower,
                    0.01f,
                    0.01f,
                    10.0f
                );

                if (ImGui::Button("Reset Primitive")) {
                    settings =
                        Primitive::Settings{};
                }
            }


            if (cylinder_) {
                Cylinder::Settings& settings =
                    cylinder_->GetSettings();

                ImGui::SeparatorText("Cylinder");

                ImGui::ColorEdit4(
                    "Cylinder Start Color",
                    &settings.color.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::ColorEdit4(
                    "Cylinder End Color",
                    &settings.endColor.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::DragFloat(
                    "Cylinder Glow",
                    &settings.intensity,
                    0.01f,
                    0.0f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Cylinder Start Radius",
                    &settings.radius,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Cylinder End Radius",
                    &settings.endRadius,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Cylinder Start Height",
                    &settings.startHeight,
                    0.01f,
                    0.01f,
                    20.0f
                );

                ImGui::DragFloat(
                    "Cylinder End Height",
                    &settings.endHeight,
                    0.01f,
                    0.01f,
                    30.0f
                );

                ImGui::DragFloat(
                    "Cylinder Bottom Radius Scale",
                    &settings.bottomRadiusScale,
                    0.01f,
                    0.0f,
                    5.0f
                );

                ImGui::DragFloat(
                    "Cylinder Top Radius Scale",
                    &settings.topRadiusScale,
                    0.01f,
                    0.0f,
                    5.0f
                );

                ImGui::DragFloat(
                    "Cylinder Life Time",
                    &settings.lifeTime,
                    0.01f,
                    0.05f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Cylinder Rise Distance",
                    &settings.riseDistance,
                    0.01f,
                    -10.0f,
                    20.0f
                );

                ImGui::DragFloat(
                    "Cylinder Ease Power",
                    &settings.easePower,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::SliderFloat(
                    "Cylinder Fade In",
                    &settings.fadeInRatio,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloat(
                    "Cylinder Fade Power",
                    &settings.fadePower,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Cylinder Twist Amount",
                    &settings.twistAmount,
                    0.01f,
                    -20.0f,
                    20.0f
                );

                ImGui::DragFloat(
                    "Cylinder Twist Speed",
                    &settings.twistSpeed,
                    0.01f,
                    -20.0f,
                    20.0f
                );

                ImGui::SliderFloat(
                    "Cylinder Noise Strength",
                    &settings.noiseStrength,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloat(
                    "Cylinder Noise Frequency",
                    &settings.noiseFrequency,
                    0.1f,
                    1.0f,
                    32.0f
                );

                ImGui::DragFloat(
                    "Cylinder Noise Speed",
                    &settings.noiseSpeed,
                    0.01f,
                    -10.0f,
                    10.0f
                );

                ImGui::SliderFloat(
                    "Cylinder Top Fade",
                    &settings.topFade,
                    0.001f,
                    1.0f
                );

                ImGui::SliderFloat(
                    "Cylinder Bottom Fade",
                    &settings.bottomFade,
                    0.001f,
                    1.0f
                );

                ImGui::DragFloat3(
                    "Cylinder Position Offset",
                    &settings.positionOffset.x,
                    0.01f,
                    -20.0f,
                    20.0f
                );

                if (ImGui::Button("Reset Cylinder")) {
                    settings =
                        Cylinder::Settings{};
                }
            }

            ImGui::EndTabItem();
        }

        // ==================================================
        // Post Effect
        // ==================================================
        if (ImGui::BeginTabItem("PostEffect")) {

            ImGui::Text("Post Effect Settings");
            ImGui::Separator();

            bool enableGrayScale = context_->GetDxCommon()->GetGrayScale();

            if (ImGui::Checkbox("GrayScale", &enableGrayScale)) {
                context_->GetDxCommon()->SetGrayScale(enableGrayScale);
            }

            ImGui::EndTabItem();
        }

        // ==================================================
        // Environment
        // ==================================================
        if (ImGui::BeginTabItem("Environment")) {

            ImGui::Checkbox(
                "Enable Skybox",
                &enableSkybox_
            );

            ImGui::Text("Environment Lighting");
            ImGui::Separator();

            if (ImGui::SliderFloat(
                "Environment Coefficient",
                &environmentCoefficient_,
                0.0f,
                1.0f
            )) {
                for (auto& object : objects_) {
                    object->SetEnvironmentCoefficient(environmentCoefficient_);
                }

                animationDebug_.SetEnvironmentCoefficient(environmentCoefficient_);
            }

            ImGui::EndTabItem();
        }

        // ==================================================
        // Sound
        // ==================================================
        if (ImGui::BeginTabItem("Sound")) {

            soundController_.DrawImGui();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    //==============================
    // Camera Debug Window
    //==============================
    {
        Camera* camera = context_->GetCamera();

        Vector3 rotate = camera->GetRotate();
        Vector3 translate = camera->GetTranslate();

        float fov = camera->GetFovY();
        float nearClip = camera->GetNearClip();
        float farClip = camera->GetFarClip();

        ImGui::SetNextWindowPos(ImVec2(460.0f, 20.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(360.0f, 360.0f), ImGuiCond_FirstUseEver);

        ImGui::Begin("Camera Debug Panel");

        ImGui::Text("Camera Transform");
        ImGui::Separator();

        if (ImGui::BeginTabBar("CameraTabs")) {

            if (ImGui::BeginTabItem("Transform")) {

                ImGui::Text("Position");
                ImGui::DragFloat3("Translate", &translate.x, 0.1f);

                ImGui::Spacing();

                ImGui::Text("Rotation");
                ImGui::DragFloat3("Rotate", &rotate.x, 0.01f);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Lens")) {

                ImGui::Text("Projection");
                ImGui::DragFloat("Fov", &fov, 0.01f, 0.1f, 2.0f);
                ImGui::DragFloat("Near Clip", &nearClip, 0.01f, 0.01f, 10.0f);
                ImGui::DragFloat("Far Clip", &farClip, 1.0f, 10.0f, 10000.0f);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Preset")) {

                if (ImGui::Button("Default Camera", ImVec2(220.0f, 32.0f))) {
                    translate = { 0.0f, 5.0f, -10.0f };
                    rotate = { 0.3f, 0.0f, 0.0f };
                    fov = 0.45f;
                    nearClip = 0.1f;
                    farClip = 100.0f;
                }

                if (ImGui::Button("Terrain Check Camera", ImVec2(220.0f, 32.0f))) {
                    translate = { 0.0f, 10.0f, -30.0f };
                    rotate = { 0.35f, 0.0f, 0.0f };
                    fov = 0.45f;
                    nearClip = 0.1f;
                    farClip = 500.0f;
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::Text("Current");
        ImGui::BulletText("Pos  : %.2f, %.2f, %.2f", translate.x, translate.y, translate.z);
        ImGui::BulletText("Rot  : %.2f, %.2f, %.2f", rotate.x, rotate.y, rotate.z);
        ImGui::BulletText("Fov  : %.2f", fov);

        camera->SetTranslate(translate);
        camera->SetRotate(rotate);
        camera->SetFovY(fov);
        camera->SetNearClip(nearClip);
        camera->SetFarClip(farClip);

        ImGui::End();
    }

    ImGui::End();

    context_->GetImGuiManager()->End();
#endif
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

    if (drawMode_ == DrawMode::NormalObj) {
        for (auto& object : objects_) {
            object->Draw();
        }
    }

    if (drawMode_ == DrawMode::Animation) {
        animationDebug_.Draw();
    }

    if (enableSkybox_ && skybox_) {
        skybox_->Draw();
    }

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
}

void GameScene::Draw2D() {
    DirectXCommon* dxCommon = context_->GetDxCommon();

    dxCommon->PreDraw();

    dxCommon->DrawRenderTextureToSwapChain();

    context_->GetSrvManager()->PreDraw();

    context_->GetSpriteCommon()->PreDraw();

    if (sprite_ && showDebugSprite_) {
        sprite_->Draw();
    }

    context_->GetImGuiManager()->Draw();

    dxCommon->PostDraw();
}
