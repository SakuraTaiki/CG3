#include "GameScene.h"

#include "ModelManager.h"
#include "AnimationLoader.h"

#include "Input.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "ImGuiManager.h"
#include "camera.h"


#include <cmath>

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
    InitializeSound();
    InitializeRing();
    InitializeCylinder();
    InitializePrimitive();
}

void GameScene::Finalize() {
    objects_.clear();
    skeletonDebugObjects_.clear();
    skeletonBoneObjects_.clear();

    skybox_.reset();
    sprite_.reset();

    sound_.Finalize();
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

    {
        Model* modelAnimated =
            ModelManager::Load("Resources/human", "walk.gltf");

        animatedCubeAnimation_ =
            AnimationLoader::Load("Resources/human", "walk.gltf");

        animatedSkeleton_ =
            CreateSkeleton(modelAnimated->GetRootNode());

        InitializeSkeletonDebug();

        animatedObject_ = std::make_unique<Object3d>();
        animatedObject_->Initialize(object3dCommon);
        animatedObject_->SetModel(modelAnimated);
        //animatedObject_->SetAnimation(animatedCubeAnimation_);
        animatedObject_->SetSkeleton(animatedSkeleton_);

        animatedObject_->SetPosition({ 0.0f, 0.0f, 0.0f });
        animatedObject_->SetRotation({ 0.0f, 0.0f, 0.0f });
        animatedObject_->SetScale({ 1.0f, 1.0f, 1.0f });

        animatedObject_->SetEnvironmentTexture(environmentTexturehandle_);
        animatedObject_->SetEnvironmentCoefficient(environmentCoefficient_);
    }
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

void GameScene::InitializeSound() {
    sound_.Initialize();

    wavSoundData_ = sound_.SoundLoadFile("Resources/Sound/Alarm01.wav");
    mp4SoundData_ = sound_.SoundLoadFile("Resources/Sound/AlarmMovie.mp4");
    mp3SoundData_ = sound_.SoundLoadFile("Resources/Sound/maou_bgm_neorock83.mp3");
}

void GameScene::InitializeRing() {
    ring_ = std::make_unique<Ring>();
    ring_->Initialize(context_->GetDxCommon(), context_->GetTextureManager());
    ring_->SetIsActive(enableRing_);
}

void GameScene::InitializeCylinder()
{
    cylinder_ = std::make_unique<Cylinder>();
    cylinder_->Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager()
    );
    cylinder_->SetIsActive(enableCylinder_);

}

void GameScene::InitializePrimitive()
{
    primitive_ = std::make_unique<Primitive>();

    primitive_->Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager()
    );

    primitive_->SetIsActive(
        enablePrimitive_
    );
}

void GameScene::InitializeSkeletonDebug() {
    skeletonDebugObjects_.clear();
    skeletonBoneObjects_.clear();

    Object3dCommon* object3dCommon = context_->GetObject3dCommon();
    Model* jointModel = ModelManager::Load("axis.obj");

    for (size_t i = 0; i < animatedSkeleton_.joints.size(); ++i) {
        auto jointObj = std::make_unique<Object3d>();
        jointObj->Initialize(object3dCommon);
        jointObj->SetModel(jointModel);
        jointObj->SetScale({ 0.3f, 0.3f, 0.3f });
        jointObj->SetRotation({ 0.0f, 0.0f, 0.0f });
        jointObj->SetPosition({ 0.0f, 0.0f, 0.0f });
        jointObj->SetEnvironmentTexture(environmentTexturehandle_);
        jointObj->SetEnvironmentCoefficient(environmentCoefficient_);

        skeletonDebugObjects_.push_back(std::move(jointObj));
    }

    for (const Joint& joint : animatedSkeleton_.joints) {
        if (!joint.parent) {
            continue;
        }

        auto boneObj = std::make_unique<Object3d>();
        boneObj->Initialize(object3dCommon);
        boneObj->SetModel(jointModel);
        boneObj->SetScale({ 0.1f, 0.1f, 0.1f });
        boneObj->SetRotation({ 0.0f, 0.0f, 0.0f });
        boneObj->SetPosition({ 0.0f, 0.0f, 0.0f });
        boneObj->SetEnvironmentTexture(environmentTexturehandle_);
        boneObj->SetEnvironmentCoefficient(environmentCoefficient_);

        skeletonBoneObjects_.push_back(std::move(boneObj));
    }
}

void GameScene::EmitEffect(
    const Vector3& position
) {
    // 新しく作成したHitEffect
    context_->GetParticleManager()->Emit(
        position,
        28
    );

    // 元のPrimitive
    if (enablePrimitive_ && primitive_) {
        primitive_->Emit(position);
    }

    if (enableRing_ && ring_) {
        ring_->Emit(position);
    }

    if (enableCylinder_ && cylinder_) {
        cylinder_->Emit(position);
    }
}

void GameScene::Update() {
    Input* input = context_->GetInput();

    if (input->TriggerKey(DIK_SPACE)) {
        EmitEffect({ 0.0f, 3.0f, 0.0f });
    }

    UpdateSound();
    UpdateObjects();

    context_->GetCamera()->Update();

    const Matrix4x4& view = context_->GetCamera()->GetViewMatrix();
    const Matrix4x4& projection = context_->GetCamera()->GetProjectionMatrix();

    if (skybox_) {
        skybox_->SetCamera(view, projection);
        skybox_->Update();
    }

    if (sprite_) {
        sprite_->Update();
    }

    if (ring_) {
        ring_->SetIsActive(enableRing_);
        ring_->Update(view, projection);
    }

    if (cylinder_) {
        cylinder_->SetIsActive(enableCylinder_);
        cylinder_->Update(view, projection);
    }

    if (primitive_) {
        primitive_->SetIsActive(
            enablePrimitive_
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
        UpdateAnimationDebug();

        if (showSkeletonDebug_) {
            UpdateSkeletonDebug();
        }

        SyncAnimatedSkeletonToObject();

        if (animatedObject_) {
            animatedObject_->Update();
        }
    }
}


void GameScene::UpdateAnimationDebug() {
    if (animationPlaying_) {
        skeletonAnimationTime_ += (1.0f / 60.0f) * animationSpeed_;

        if (animatedCubeAnimation_.duration > 0.0f) {
            if (animationLoop_) {
                skeletonAnimationTime_ =
                    std::fmod(skeletonAnimationTime_, animatedCubeAnimation_.duration);
            } else if (skeletonAnimationTime_ > animatedCubeAnimation_.duration) {
                skeletonAnimationTime_ = animatedCubeAnimation_.duration;
                animationPlaying_ = false;
            }
        }

        ApplyAnimation(
            animatedSkeleton_,
            animatedCubeAnimation_,
            skeletonAnimationTime_
        );
    }

    UpdateSkelton(animatedSkeleton_);
}

void GameScene::SyncAnimatedSkeletonToObject() {
    if (!animatedObject_) {
        return;
    }

    animatedObject_->SetSkeleton(animatedSkeleton_);
}

void GameScene::UpdateSound() {
    Input* input = context_->GetInput();

    if (input->TriggerKey(DIK_M)) {
        sound_.SoundPlay(mp4SoundData_, mp4Volume_);
    }

    if (input->TriggerKey(DIK_N)) {
        sound_.SoundPlay(mp3SoundData_, mp3Volume_);
    }

    if (input->TriggerKey(DIK_UP)) {
        mp3Volume_ += 0.1f;
        if (mp3Volume_ > 1.0f) {
            mp3Volume_ = 1.0f;
        }
    }

    if (input->TriggerKey(DIK_DOWN)) {
        mp3Volume_ -= 0.1f;
        if (mp3Volume_ < 0.0f) {
            mp3Volume_ = 0.0f;
        }
    }
}

void GameScene::DrawAnimationDebugImGui() {
#ifdef USE_IMGUI
    ImGui::Text("Animation Control");
    ImGui::Separator();

    if (ImGui::Button(animationPlaying_ ? "Pause" : "Play", ImVec2(100.0f, 28.0f))) {
        animationPlaying_ = !animationPlaying_;
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset", ImVec2(100.0f, 28.0f))) {
        skeletonAnimationTime_ = 0.0f;
        ApplyAnimation(animatedSkeleton_, animatedCubeAnimation_, skeletonAnimationTime_);
        UpdateSkelton(animatedSkeleton_);
    }

    ImGui::Checkbox("Loop", &animationLoop_);
    ImGui::Checkbox("Auto Pause On Joint Edit", &autoPauseOnJointEdit_);
    ImGui::Checkbox("Show Skeleton Debug", &showSkeletonDebug_);

    ImGui::DragFloat("Speed", &animationSpeed_, 0.01f, 0.0f, 5.0f);

    if (animatedCubeAnimation_.duration > 0.0f) {
        ImGui::SliderFloat(
            "Time",
            &skeletonAnimationTime_,
            0.0f,
            animatedCubeAnimation_.duration
        );

        if (!animationPlaying_) {
            ApplyAnimation(animatedSkeleton_, animatedCubeAnimation_, skeletonAnimationTime_);
            UpdateSkelton(animatedSkeleton_);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Skeleton");

    const int jointCount = static_cast<int>(animatedSkeleton_.joints.size());
    ImGui::Text("Joint Count : %d", jointCount);

    if (jointCount == 0) {
        return;
    }

    if (selectedJointIndex_ < 0) {
        selectedJointIndex_ = 0;
    }

    if (selectedJointIndex_ >= jointCount) {
        selectedJointIndex_ = jointCount - 1;
    }

    const char* previewName =
        animatedSkeleton_.joints[selectedJointIndex_].name.c_str();

    if (ImGui::BeginCombo("Selected Joint", previewName)) {
        for (int i = 0; i < jointCount; ++i) {
            const bool isSelected = selectedJointIndex_ == i;

            std::string label =
                std::to_string(i) + " : " + animatedSkeleton_.joints[i].name;

            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedJointIndex_ = i;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    Joint& joint = animatedSkeleton_.joints[selectedJointIndex_];

    ImGui::Spacing();
    ImGui::Text("Joint Detail");
    ImGui::Separator();

    ImGui::Text("Name   : %s", joint.name.c_str());
    ImGui::Text("Index  : %d", joint.index);

    if (joint.parent) {
        ImGui::Text("Parent : %d", *joint.parent);
    } else {
        ImGui::Text("Parent : none");
    }

    Vector3 translate = joint.transform.translate;
    Vector3 scale = joint.transform.scale;
    Quaternion rotate = joint.transform.rotate;

    bool edited = false;

    edited |= ImGui::DragFloat3(
        "Local Translate",
        &translate.x,
        0.01f,
        -100.0f,
        100.0f
    );

    edited |= ImGui::DragFloat3(
        "Local Scale",
        &scale.x,
        0.01f,
        0.001f,
        100.0f
    );

    edited |= ImGui::DragFloat4(
        "Local Rotate Quaternion",
        &rotate.x,
        0.01f,
        -1.0f,
        1.0f
    );

    if (ImGui::Button("Normalize Quaternion", ImVec2(180.0f, 28.0f))) {
        rotate = Math::Normalize(rotate);
        edited = true;
    }

    if (edited) {
        if (autoPauseOnJointEdit_) {
            animationPlaying_ = false;
        }

        joint.transform.translate = translate;
        joint.transform.scale = scale;
        joint.transform.rotate = Math::Normalize(rotate);

        UpdateSkelton(animatedSkeleton_);
    }

    const Matrix4x4& skeletonMatrix = joint.skeletonSpaceMatrix;

    Vector3 skeletonPosition = {
        skeletonMatrix.m[3][0],
        skeletonMatrix.m[3][1],
        skeletonMatrix.m[3][2]
    };

    ImGui::Spacing();
    ImGui::Text("Skeleton Space");
    ImGui::BulletText(
        "Position : %.3f, %.3f, %.3f",
        skeletonPosition.x,
        skeletonPosition.y,
        skeletonPosition.z
    );

    if (ImGui::TreeNode("Children")) {
        for (int32_t childIndex : joint.children) {
            const Joint& child = animatedSkeleton_.joints[childIndex];
            ImGui::BulletText("%d : %s", childIndex, child.name.c_str());
        }

        ImGui::TreePop();
    }
#endif
}

void GameScene::UpdateImGui() {
#ifdef USE_IMGUI
    context_->GetImGuiManager()->Begin();

    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("CG2 Effect Debug Panel");

    ImGui::Text("Scene Control");
    ImGui::Separator();

    if (ImGui::Button("Emit Effect SPACE", ImVec2(180.0f, 32.0f))) {
        EmitEffect({ 0.0f, 3.0f, 0.0f });
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
    ImGui::BulletText("Joint Count : %d", static_cast<int>(animatedSkeleton_.joints.size()));
    ImGui::BulletText("Animation Time : %.2f", skeletonAnimationTime_);
    ImGui::Spacing();

    ImGui::Checkbox("Show Skeleton Debug", &showSkeletonDebug_);

    ImGui::BulletText(
        "Joint Count : %d",
        static_cast<int>(animatedSkeleton_.joints.size())
    );

    if (ImGui::BeginTabBar("MainTabs")) {

        if (ImGui::BeginTabItem("Animation")) {
            DrawAnimationDebugImGui();
            ImGui::EndTabItem();
        }

        // ==================================================
        // Effect
        // ==================================================
        if (ImGui::BeginTabItem("Effect")) {
            ImGui::Checkbox(
                "Enable Primitive",
                &enablePrimitive_
            );

            ImGui::Checkbox(
                "Enable Ring",
                &enableRing_
            );

            ImGui::Checkbox(
                "Enable Cylinder",
                &enableCylinder_
            );

            ImGui::SeparatorText("Emit");

            ImGui::DragFloat3(
                "Effect Position",
                &hitEffectPosition_.x,
                0.05f,
                -20.0f,
                20.0f
            );

            if (ImGui::Button(
                "Emit Effect",
                ImVec2(180.0f, 32.0f)
            )) {
                EmitEffect(hitEffectPosition_);
            }

            if (ring_) {
                Ring::Settings& settings =
                    ring_->GetSettings();

                ImGui::SeparatorText("Ring");

                ImGui::ColorEdit4(
                    "Ring Color",
                    &settings.color.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::DragFloat(
                    "Glow Intensity",
                    &settings.intensity,
                    0.01f,
                    0.0f,
                    5.0f
                );

                ImGui::DragFloat(
                    "Start Scale",
                    &settings.startScale,
                    0.01f,
                    0.01f,
                    10.0f
                );

                ImGui::DragFloat(
                    "End Scale",
                    &settings.endScale,
                    0.01f,
                    0.01f,
                    20.0f
                );

                float thickness =
                    settings.thickness;

                if (ImGui::SliderFloat(
                    "Ring Thickness",
                    &thickness,
                    0.02f,
                    0.95f
                )) {
                    ring_->SetThickness(thickness);
                }

                ImGui::DragFloat(
                    "Life Time",
                    &settings.lifeTime,
                    0.01f,
                    0.05f,
                    5.0f
                );

                ImGui::SliderFloat(
                    "Fade In Ratio",
                    &settings.fadeInRatio,
                    0.0f,
                    1.0f
                );

                ImGui::DragFloat(
                    "Expand Ease Power",
                    &settings.easePower,
                    0.05f,
                    0.1f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Rotation Speed",
                    &settings.rotationSpeed,
                    0.05f,
                    -20.0f,
                    20.0f
                );

                if (ImGui::Button("Reset Ring")) {
                    settings = Ring::Settings{};
                    ring_->SetThickness(
                        settings.thickness
                    );
                }
            }


            if (primitive_) {
                Primitive::Settings& settings =
                    primitive_->GetSettings();

                ImGui::SeparatorText("Primitive");

                ImGui::ColorEdit4(
                    "Primitive Color",
                    &settings.color.x,
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
                    5.0f
                );

                ImGui::DragFloat(
                    "Primitive Width",
                    &settings.width,
                    0.001f,
                    0.001f,
                    2.0f
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

                ImGui::DragFloat(
                    "Primitive Rotation Speed",
                    &settings.rotationSpeed,
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
                    "Primitive Fade Power",
                    &settings.fadePower,
                    0.01f,
                    0.1f,
                    10.0f
                );

                if (ImGui::Button("Reset Primitive")) {
                    settings = Primitive::Settings{};
                }
            }


            if (cylinder_) {
                Cylinder::Settings& settings =
                    cylinder_->GetSettings();

                ImGui::SeparatorText("Cylinder");

                ImGui::ColorEdit4(
                    "Cylinder Color",
                    &settings.color.x,
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_HDR
                );

                ImGui::DragFloat(
                    "Cylinder Glow",
                    &settings.intensity,
                    0.01f,
                    0.0f,
                    5.0f
                );

                ImGui::DragFloat(
                    "Cylinder Radius",
                    &settings.radius,
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
                    "Cylinder Expand Ease",
                    &settings.easePower,
                    0.01f,
                    0.1f,
                    10.0f
                );

                ImGui::DragFloat(
                    "Cylinder Fade Power",
                    &settings.fadePower,
                    0.01f,
                    0.1f,
                    10.0f
                );

                ImGui::DragFloat3(
                    "Cylinder Position Offset",
                    &settings.positionOffset.x,
                    0.01f,
                    -20.0f,
                    20.0f
                );

                if (ImGui::Button("Reset Cylinder")) {
                    settings = Cylinder::Settings{};
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
            }

            ImGui::EndTabItem();
        }

        // ==================================================
        // Sound
        // ==================================================
        if (ImGui::BeginTabItem("Sound")) {

            ImGui::Text("Sound Volume");
            ImGui::Separator();

            ImGui::SliderFloat("Wav Volume", &wavVolume_, 0.0f, 1.0f);
            ImGui::SliderFloat("Mp4 Volume", &mp4Volume_, 0.0f, 1.0f);
            ImGui::SliderFloat("Mp3 Volume", &mp3Volume_, 0.0f, 1.0f);

            ImGui::Spacing();
            ImGui::TextDisabled("M key : Play mp4");
            ImGui::TextDisabled("N key : Play mp3");
            ImGui::TextDisabled("UP / DOWN : Change mp3 volume");

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

void GameScene::UpdateSkeletonDebug() {
    if (skeletonDebugObjects_.size() != animatedSkeleton_.joints.size()) {
        return;
    }

    for (size_t i = 0; i < animatedSkeleton_.joints.size(); ++i) {
        const Matrix4x4& mat = animatedSkeleton_.joints[i].skeletonSpaceMatrix;

        Vector3 jointPosition = {
            mat.m[3][0],
            mat.m[3][1],
            mat.m[3][2]
        };

        skeletonDebugObjects_[i]->SetPosition(jointPosition);
        skeletonDebugObjects_[i]->Update();
    }

    size_t boneIndex = 0;

    for (const Joint& joint : animatedSkeleton_.joints) {
        if (!joint.parent) {
            continue;
        }

        const Matrix4x4& childMat = joint.skeletonSpaceMatrix;
        const Matrix4x4& parentMat =
            animatedSkeleton_.joints[*joint.parent].skeletonSpaceMatrix;

        Vector3 childPos = {
            childMat.m[3][0],
            childMat.m[3][1],
            childMat.m[3][2]
        };

        Vector3 parentPos = {
            parentMat.m[3][0],
            parentMat.m[3][1],
            parentMat.m[3][2]
        };

        Vector3 center = {
            (childPos.x + parentPos.x) * 0.5f,
            (childPos.y + parentPos.y) * 0.5f,
            (childPos.z + parentPos.z) * 0.5f
        };

        skeletonBoneObjects_[boneIndex]->SetPosition(center);
        skeletonBoneObjects_[boneIndex]->SetScale({ 0.15f, 0.15f, 0.15f });
        skeletonBoneObjects_[boneIndex]->Update();

        ++boneIndex;
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

    if (drawMode_ == DrawMode::NormalObj) {
        for (auto& object : objects_) {
            object->Draw();
        }
    }

    if (drawMode_ == DrawMode::Animation) {
        if (animatedObject_) {
            animatedObject_->Draw();
        }

        if (showSkeletonDebug_) {
            for (auto& debugObject : skeletonDebugObjects_) {
                debugObject->Draw();
            }

            for (auto& boneObject : skeletonBoneObjects_) {
                boneObject->Draw();
            }
        }
    }

    if (skybox_) {
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

    if (sprite_&&showDebugSprite_) {
        sprite_->Draw();
    }

    context_->GetImGuiManager()->Draw();

    dxCommon->PostDraw();
}