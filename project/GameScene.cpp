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
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>


namespace
{
    std::string MakeSafePresetName(
        const std::string& name
    ) {
        std::string result;

        for (unsigned char character : name) {
            if (
                std::isalnum(character) ||
                character == '_' ||
                character == '-'
                ) {
                result +=
                    static_cast<char>(character);
            }
        }

        if (result.empty()) {
            result = "NewPreset";
        }

        return result;
    }
}


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

    ApplyFireHitEffectPreset();

    RefreshEffectPresetList();
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
    const float size =
        (std::max)(hitEffectSize_, 0.01f);

    // 元から存在するHitEffect
    context_->GetParticleManager()->Emit(
        position,
        28
    );

    // =====================================
    // Primitive
    // =====================================
    if (enablePrimitive_ && primitive_) {
        Primitive::Settings& settings =
            primitive_->GetSettings();

        // 現在の設定を退避
        const float originalWidth =
            settings.width;

        const float originalMinLength =
            settings.minLength;

        const float originalMaxLength =
            settings.maxLength;

        const float originalMoveSpeed =
            settings.moveSpeed;

        // サイズ倍率を適用
        settings.width =
            originalWidth * size;

        settings.minLength =
            originalMinLength * size;

        settings.maxLength =
            originalMaxLength * size;

        settings.moveSpeed =
            originalMoveSpeed * size;

        primitive_->Emit(position);

        // ImGui上の設定値を元に戻す
        settings.width =
            originalWidth;

        settings.minLength =
            originalMinLength;

        settings.maxLength =
            originalMaxLength;

        settings.moveSpeed =
            originalMoveSpeed;
    }

    // =====================================
    // Ring
    // =====================================
    if (enableRing_ && ring_) {
        Ring::Settings& settings =
            ring_->GetSettings();

        // 現在の設定を退避
        const float originalStartScale =
            settings.startScale;

        const float originalEndScale =
            settings.endScale;

        // サイズ倍率を適用
        settings.startScale =
            originalStartScale * size;

        settings.endScale =
            originalEndScale * size;

        ring_->Emit(position);

        // ImGui上の設定値を元に戻す
        settings.startScale =
            originalStartScale;

        settings.endScale =
            originalEndScale;
    }

    // =====================================
    // Cylinder
    // =====================================
    if (enableCylinder_ && cylinder_) {
        Cylinder::Settings& settings =
            cylinder_->GetSettings();

        // 現在の設定を退避
        const float originalRadius =
            settings.radius;

        const float originalStartHeight =
            settings.startHeight;

        const float originalEndHeight =
            settings.endHeight;

        const float originalRiseDistance =
            settings.riseDistance;

        const Vector3 originalPositionOffset =
            settings.positionOffset;

        // サイズ倍率を適用
        settings.radius =
            originalRadius * size;

        settings.startHeight =
            originalStartHeight * size;

        settings.endHeight =
            originalEndHeight * size;

        settings.riseDistance =
            originalRiseDistance * size;

        settings.positionOffset = {
            originalPositionOffset.x * size,
            originalPositionOffset.y * size,
            originalPositionOffset.z * size
        };

        cylinder_->Emit(position);

        // ImGui上の設定値を元に戻す
        settings.radius =
            originalRadius;

        settings.startHeight =
            originalStartHeight;

        settings.endHeight =
            originalEndHeight;

        settings.riseDistance =
            originalRiseDistance;

        settings.positionOffset =
            originalPositionOffset;
    }
}

bool GameScene::SaveEffectPreset(const std::string& presetName)
{
    namespace fs = std::filesystem;

    std::string safeName;

    for (unsigned char character : presetName) {
        if (
            std::isalnum(character) ||
            character == '_' ||
            character == '-'
            ) {
            safeName += static_cast<char>(character);
        }
    }

    if (safeName.empty()) {
        safeName = "NewPreset";
    }

    const fs::path directory =
        "Resources/Settings/HitEffects";

    fs::create_directories(directory);

    const fs::path filePath =
        directory / (safeName + ".txt");

    std::ofstream file(filePath);

    if (!file.is_open()) {
        effectSettingsMessage_ =
            "Preset save failed";

        return false;
    }

    file << "EffectPosition "
        << hitEffectPosition_.x << " "
        << hitEffectPosition_.y << " "
        << hitEffectPosition_.z << "\n";

    file << "EffectSize "
        << hitEffectSize_ << "\n";

    file << "EnablePrimitive "
        << enablePrimitive_ << "\n";

    file << "EnableRing "
        << enableRing_ << "\n";

    file << "EnableCylinder "
        << enableCylinder_ << "\n";

    if (primitive_) {
        const Primitive::Settings& settings =
            primitive_->GetSettings();

        file << "PrimitiveColor "
            << settings.color.x << " "
            << settings.color.y << " "
            << settings.color.z << " "
            << settings.color.w << "\n";

        file << "PrimitiveCount "
            << settings.count << "\n";

        file << "PrimitiveIntensity "
            << settings.intensity << "\n";

        file << "PrimitiveWidth "
            << settings.width << "\n";

        file << "PrimitiveLength "
            << settings.minLength << " "
            << settings.maxLength << "\n";

        file << "PrimitiveLifeTime "
            << settings.minLifeTime << " "
            << settings.maxLifeTime << "\n";

        file << "PrimitiveMoveSpeed "
            << settings.moveSpeed << "\n";

        file << "PrimitiveRotationSpeed "
            << settings.rotationSpeed << "\n";

        file << "PrimitiveEndScale "
            << settings.endWidthScale << " "
            << settings.endLengthScale << "\n";

        file << "PrimitiveFadePower "
            << settings.fadePower << "\n";
    }

    if (ring_) {
        const Ring::Settings& settings =
            ring_->GetSettings();

        file << "RingColor "
            << settings.color.x << " "
            << settings.color.y << " "
            << settings.color.z << " "
            << settings.color.w << "\n";

        file << "RingIntensity "
            << settings.intensity << "\n";

        file << "RingScale "
            << settings.startScale << " "
            << settings.endScale << "\n";

        file << "RingThickness "
            << settings.thickness << "\n";

        file << "RingLifeTime "
            << settings.lifeTime << "\n";

        file << "RingFadeInRatio "
            << settings.fadeInRatio << "\n";

        file << "RingEasePower "
            << settings.easePower << "\n";

        file << "RingRotationSpeed "
            << settings.rotationSpeed << "\n";
    }

    if (cylinder_) {
        const Cylinder::Settings& settings =
            cylinder_->GetSettings();

        file << "CylinderColor "
            << settings.color.x << " "
            << settings.color.y << " "
            << settings.color.z << " "
            << settings.color.w << "\n";

        file << "CylinderIntensity "
            << settings.intensity << "\n";

        file << "CylinderRadius "
            << settings.radius << "\n";

        file << "CylinderHeight "
            << settings.startHeight << " "
            << settings.endHeight << "\n";

        file << "CylinderLifeTime "
            << settings.lifeTime << "\n";

        file << "CylinderRiseDistance "
            << settings.riseDistance << "\n";

        file << "CylinderEasePower "
            << settings.easePower << "\n";

        file << "CylinderFadePower "
            << settings.fadePower << "\n";

        file << "CylinderPositionOffset "
            << settings.positionOffset.x << " "
            << settings.positionOffset.y << " "
            << settings.positionOffset.z << "\n";
    }

    file.close();

    effectSettingsMessage_ =
        "Saved : " + safeName;

    RefreshEffectPresetList();

    return true;
}

bool GameScene::LoadEffectPreset(const std::string& presetName)
{
    namespace fs = std::filesystem;

    std::string safeName;

    for (unsigned char character : presetName) {
        if (
            std::isalnum(character) ||
            character == '_' ||
            character == '-'
            ) {
            safeName += static_cast<char>(character);
        }
    }

    if (safeName.empty()) {
        safeName = "NewPreset";
    }

    const fs::path filePath =
        fs::path("Resources/Settings/HitEffects") /
        (safeName + ".txt");

    std::ifstream file(filePath);

    if (!file.is_open()) {
        effectSettingsMessage_ =
            "Preset not found : " + safeName;

        return false;
    }

    std::string key;

    while (file >> key) {
        if (key == "EffectPosition") {
            file
                >> hitEffectPosition_.x
                >> hitEffectPosition_.y
                >> hitEffectPosition_.z;
        } else if (key == "EffectSize") {
            file >> hitEffectSize_;

            hitEffectSize_ =
                std::clamp(
                    hitEffectSize_,
                    0.1f,
                    5.0f
                );
        } else if (key == "EnablePrimitive") {
            file >> enablePrimitive_;
        } else if (key == "EnableRing") {
            file >> enableRing_;
        } else if (key == "EnableCylinder") {
            file >> enableCylinder_;
        } else if (
            key == "PrimitiveColor" &&
            primitive_
            ) {
            Primitive::Settings& settings =
                primitive_->GetSettings();

            file
                >> settings.color.x
                >> settings.color.y
                >> settings.color.z
                >> settings.color.w;
        } else if (
            key == "PrimitiveCount" &&
            primitive_
            ) {
            file >> primitive_->GetSettings().count;
        } else if (
            key == "PrimitiveIntensity" &&
            primitive_
            ) {
            file >> primitive_->GetSettings().intensity;
        } else if (
            key == "PrimitiveWidth" &&
            primitive_
            ) {
            file >> primitive_->GetSettings().width;
        } else if (
            key == "PrimitiveLength" &&
            primitive_
            ) {
            Primitive::Settings& settings =
                primitive_->GetSettings();

            file
                >> settings.minLength
                >> settings.maxLength;
        } else if (
            key == "PrimitiveLifeTime" &&
            primitive_
            ) {
            Primitive::Settings& settings =
                primitive_->GetSettings();

            file
                >> settings.minLifeTime
                >> settings.maxLifeTime;
        } else if (
            key == "PrimitiveMoveSpeed" &&
            primitive_
            ) {
            file >> primitive_->GetSettings().moveSpeed;
        } else if (
            key == "PrimitiveRotationSpeed" &&
            primitive_
            ) {
            file >> primitive_->GetSettings().rotationSpeed;
        } else if (
            key == "PrimitiveEndScale" &&
            primitive_
            ) {
            Primitive::Settings& settings =
                primitive_->GetSettings();

            file
                >> settings.endWidthScale
                >> settings.endLengthScale;
        } else if (
            key == "PrimitiveFadePower" &&
            primitive_
            ) {
            file >> primitive_->GetSettings().fadePower;
        } else if (
            key == "RingColor" &&
            ring_
            ) {
            Ring::Settings& settings =
                ring_->GetSettings();

            file
                >> settings.color.x
                >> settings.color.y
                >> settings.color.z
                >> settings.color.w;
        } else if (
            key == "RingIntensity" &&
            ring_
            ) {
            file >> ring_->GetSettings().intensity;
        } else if (
            key == "RingScale" &&
            ring_
            ) {
            Ring::Settings& settings =
                ring_->GetSettings();

            file
                >> settings.startScale
                >> settings.endScale;
        } else if (
            key == "RingThickness" &&
            ring_
            ) {
            float thickness = 0.18f;
            file >> thickness;
            ring_->SetThickness(thickness);
        } else if (
            key == "RingLifeTime" &&
            ring_
            ) {
            file >> ring_->GetSettings().lifeTime;
        } else if (
            key == "RingFadeInRatio" &&
            ring_
            ) {
            file >> ring_->GetSettings().fadeInRatio;
        } else if (
            key == "RingEasePower" &&
            ring_
            ) {
            file >> ring_->GetSettings().easePower;
        } else if (
            key == "RingRotationSpeed" &&
            ring_
            ) {
            file >> ring_->GetSettings().rotationSpeed;
        } else if (
            key == "CylinderColor" &&
            cylinder_
            ) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();

            file
                >> settings.color.x
                >> settings.color.y
                >> settings.color.z
                >> settings.color.w;
        } else if (
            key == "CylinderIntensity" &&
            cylinder_
            ) {
            file >> cylinder_->GetSettings().intensity;
        } else if (
            key == "CylinderRadius" &&
            cylinder_
            ) {
            file >> cylinder_->GetSettings().radius;
        } else if (
            key == "CylinderHeight" &&
            cylinder_
            ) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();

            file
                >> settings.startHeight
                >> settings.endHeight;
        } else if (
            key == "CylinderLifeTime" &&
            cylinder_
            ) {
            file >> cylinder_->GetSettings().lifeTime;
        } else if (
            key == "CylinderRiseDistance" &&
            cylinder_
            ) {
            file >> cylinder_->GetSettings().riseDistance;
        } else if (
            key == "CylinderEasePower" &&
            cylinder_
            ) {
            file >> cylinder_->GetSettings().easePower;
        } else if (
            key == "CylinderFadePower" &&
            cylinder_
            ) {
            file >> cylinder_->GetSettings().fadePower;
        } else if (
            key == "CylinderPositionOffset" &&
            cylinder_
            ) {
            Vector3& offset =
                cylinder_->GetSettings().positionOffset;

            file
                >> offset.x
                >> offset.y
                >> offset.z;
        } else {
            std::string unusedLine;
            std::getline(file, unusedLine);
        }
    }

    if (primitive_) {
        primitive_->SetIsActive(enablePrimitive_);
    }

    if (ring_) {
        ring_->SetIsActive(enableRing_);
    }

    if (cylinder_) {
        cylinder_->SetIsActive(enableCylinder_);
    }

    effectSettingsMessage_ =
        "Loaded : " + safeName;

    return true;
}

void GameScene::RefreshEffectPresetList()
{

    namespace fs = std::filesystem;

    const fs::path directory =
        "Resources/Settings/HitEffects";

    fs::create_directories(directory);

    effectPresetNames_.clear();

    for (
        const fs::directory_entry& entry :
        fs::directory_iterator(directory)
        ) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (entry.path().extension() != ".txt") {
            continue;
        }

        effectPresetNames_.push_back(
            entry.path().stem().string()
        );
    }

    std::sort(
        effectPresetNames_.begin(),
        effectPresetNames_.end()
    );

    if (effectPresetNames_.empty()) {
        selectedEffectPreset_ = 0;
    } else {
        selectedEffectPreset_ =
            std::clamp(
                selectedEffectPreset_,
                0,
                static_cast<int>(
                    effectPresetNames_.size()
                    ) - 1
            );
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

void GameScene::ApplyFireHitEffectPreset()
{

    // =====================================
    // Primitive：炎の筋
    // =====================================
    if (primitive_) {
        Primitive::Settings& settings =
            primitive_->GetSettings();

        settings.color = {
            1.0f,
            0.32f,
            0.02f,
            1.0f
        };

        settings.count = 18;
        settings.intensity = 1.8f;

        settings.width = 0.035f;

        settings.minLength = 0.3f;
        settings.maxLength = 1.15f;

        settings.minLifeTime = 0.18f;
        settings.maxLifeTime = 0.42f;

        settings.moveSpeed = 1.2f;
        settings.rotationSpeed = 0.0f;

        settings.endWidthScale = 0.1f;
        settings.endLengthScale = 0.45f;

        settings.fadePower = 2.2f;
    }

    // =====================================
    // Ring：炎の衝撃波
    // =====================================
    if (ring_) {
        Ring::Settings& settings =
            ring_->GetSettings();

        settings.color = {
            1.0f,
            0.22f,
            0.01f,
            0.8f
        };

        settings.intensity = 1.8f;

        settings.startScale = 0.25f;
        settings.endScale = 1.35f;

        settings.lifeTime = 0.32f;
        settings.fadeInRatio = 0.05f;

        settings.easePower = 4.0f;
        settings.rotationSpeed = 0.0f;

        ring_->SetThickness(0.12f);
    }

    // =====================================
    // Cylinder：上へ伸びる炎柱
    // =====================================
    if (cylinder_) {
        Cylinder::Settings& settings =
            cylinder_->GetSettings();

        settings.color = {
            1.0f,
            0.18f,
            0.01f,
            0.38f
        };

        settings.intensity = 1.5f;

        settings.radius = 0.48f;

        settings.startHeight = 0.05f;
        settings.endHeight = 1.8f;

        settings.lifeTime = 0.55f;

        settings.riseDistance = 0.45f;

        settings.easePower = 3.0f;
        settings.fadePower = 2.0f;

        settings.positionOffset = {
            0.0f,
            -0.15f,
            0.0f
        };
    }

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

    ImGui::Begin(
        "CG2 Effect Debug Panel",
        nullptr,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

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

            DrawHitEffectSizeImGui();


            ImGui::SeparatorText("Effect Presets");

            ImGui::InputText(
                "Preset Name",
                effectPresetNameBuffer_.data(),
                effectPresetNameBuffer_.size()
            );

            if (ImGui::Button(
                "Save As Preset",
                ImVec2(150.0f, 30.0f)
            )) {
                SaveEffectPreset(
                    effectPresetNameBuffer_.data()
                );
            }

            ImGui::SameLine();

            if (ImGui::Button(
                "Refresh Presets",
                ImVec2(150.0f, 30.0f)
            )) {
                RefreshEffectPresetList();
            }

            if (!effectPresetNames_.empty()) {
                const char* previewName =
                    effectPresetNames_[
                        selectedEffectPreset_
                    ].c_str();

                if (ImGui::BeginCombo(
                    "Saved Presets",
                    previewName
                )) {
                    for (
                        int index = 0;
                        index <
                        static_cast<int>(
                            effectPresetNames_.size()
                            );
                            ++index
                        ) {
                        const bool selected =
                            selectedEffectPreset_ ==
                            index;

                        if (ImGui::Selectable(
                            effectPresetNames_[index]
                            .c_str(),
                            selected
                        )) {
                            selectedEffectPreset_ =
                                index;
                        }

                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::Button(
                    "Load Selected Preset",
                    ImVec2(200.0f, 30.0f)
                )) {
                    LoadEffectPreset(
                        effectPresetNames_[
                            selectedEffectPreset_
                        ]
                    );
                }

                ImGui::SameLine();

                if (ImGui::Button(
                    "Load And Preview",
                    ImVec2(180.0f, 30.0f)
                )) {
                    LoadEffectPreset(
                        effectPresetNames_[
                            selectedEffectPreset_
                        ]
                    );

                    EmitEffect(
                        hitEffectPosition_
                    );
                }
            } else {
                ImGui::TextDisabled(
                    "No saved presets"
                );
            }

            if (!effectSettingsMessage_.empty()) {
                ImGui::TextDisabled(
                    "%s",
                    effectSettingsMessage_.c_str()
                );
            }

            if (!effectSettingsMessage_.empty()) {
                ImGui::TextDisabled(
                    "%s",
                    effectSettingsMessage_.c_str()
                );
            }


            if (ImGui::Button(
                "Emit Effect",
                ImVec2(180.0f, 32.0f)
            )) {
                EmitEffect(hitEffectPosition_);
            }

            if (ImGui::Button(
                "Apply Fire Preset",
                ImVec2(180.0f, 30.0f)
            )) {
                ApplyFireHitEffectPreset();
            }

            ImGui::SameLine();

            if (ImGui::Button(
                "Preview Fire",
                ImVec2(180.0f, 30.0f)
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

    if (sprite_&&showDebugSprite_) {
        sprite_->Draw();
    }

    context_->GetImGuiManager()->Draw();

    dxCommon->PostDraw();
}

void GameScene::DrawHitEffectSizeImGui()
{

#ifdef USE_IMGUI
    ImGui::SeparatorText("Hit Effect Size");

    ImGui::SliderFloat(
        "Effect Size",
        &hitEffectSize_,
        0.1f,
        5.0f,
        "%.2f"
    );

    if (ImGui::Button("Reset Effect Size")) {
        hitEffectSize_ = 1.0f;
    }

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Current : %.2f x",
        hitEffectSize_
    );
#endif


}
