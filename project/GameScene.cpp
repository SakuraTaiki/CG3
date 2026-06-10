#include "GameScene.h"
#include "ModelManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void GameScene::Initialize(GameSystem* system) {
    system_ = system;

    ModelManager::Initialize(system_->GetObject3dCommon());

    InitializeModels();
    InitializeSprite();
    InitializeSkybox();
    InitializeObjects();
    InitializeSound();
    InitializeRing();
    InitializeCylinder();
}

void GameScene::Finalize() {
    objects_.clear();

    skybox_.reset();
    sprite_.reset();

    sound_.Finalize();
    ModelManager::Finalize();

    system_ = nullptr;
}

void GameScene::InitializeModels() {
    ModelManager::Load("Resources/terrain", "terrain.obj");
    ModelManager::Load("axis.obj");
}

void GameScene::InitializeObjects() {
    Object3dCommon* object3dCommon = system_->GetObject3dCommon();

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
}

void GameScene::InitializeSprite() {
    uint32_t texHandle =
        system_->GetTextureManager()->LoadTexture("Resources/uvChecker.png");

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(system_->GetSpriteCommon(), texHandle);
}

void GameScene::InitializeSkybox() {
    skybox_ = std::make_unique<Skybox>();

    skybox_->Initialize(
        system_->GetDxCommon(),
        system_->GetTextureManager(),
        "Resources/skybox/rostock_laage_airport_4k.dds"
    );

    skybox_->SetScale({ 100.0f, 100.0f, 100.0f });

    environmentTexturehandle_ =
        system_->GetTextureManager()->LoadTexture(
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
    ring_->Initialize(system_->GetDxCommon(), system_->GetTextureManager());
    ring_->SetIsActive(enableRing_);
}

void GameScene::InitializeCylinder()
{
    cylinder_ = std::make_unique<Cylinder>();
    cylinder_->Initialize(
        system_->GetDxCommon(),
        system_->GetTextureManager()
    );
    cylinder_->SetIsActive(enableCylinder_);

}

void GameScene::Update() {
    Input* input = system_->GetInput();
    input->Update();

    if (input->TriggerKey(DIK_SPACE)) {
        Vector3 effectPos = { 0.0f, 1.0f, 0.0f };

        system_->GetParticleManager()->Emit(effectPos, 8);

        if (enableRing_ && ring_) {
            ring_->Emit(effectPos);
        }

        if (enableCylinder_ && cylinder_) {
            cylinder_->Emit(effectPos);
        }
    }

    UpdateSound();
    UpdateObjects();

    system_->GetCamera()->Update();

    const Matrix4x4& view = system_->GetCamera()->GetViewMatrix();
    const Matrix4x4& projection = system_->GetCamera()->GetProjectionMatrix();

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

    system_->GetParticleManager()->Update(view, projection);

    UpdateImGui();
}

void GameScene::UpdateObjects() {
    for (auto& object : objects_) {
        object->Update();
    }
}

void GameScene::UpdateSound() {
    Input* input = system_->GetInput();

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

void GameScene::UpdateImGui() {
#ifdef USE_IMGUI
    system_->GetImGuiManager()->Begin();

    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("CG2 Effect Debug Panel");

    ImGui::Text("Scene Control");
    ImGui::Separator();

    if (ImGui::Button("Emit Effect SPACE", ImVec2(180.0f, 32.0f))) {
        Vector3 effectPos = { 0.0f, 1.0f, 0.0f };

        system_->GetParticleManager()->Emit(effectPos, 8);

        if (enableRing_ && ring_) {
            ring_->Emit(effectPos);
        }

        if (enableCylinder_ && cylinder_) {
            cylinder_->Emit(effectPos);
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("SPACE key also emits");

    ImGui::Spacing();

    if (ImGui::BeginTabBar("MainTabs")) {

        // ==================================================
        // Effect
        // ==================================================
        if (ImGui::BeginTabItem("Effect")) {

            ImGui::Text("Primitive Effects");
            ImGui::Separator();

            ImGui::Checkbox("Enable Ring", &enableRing_);
            ImGui::Checkbox("Enable Cylinder", &enableCylinder_);

            ImGui::Spacing();

            ImGui::Text("Particle");
            ImGui::BulletText("SPACE key : Emit particle");
            ImGui::BulletText("Particle count : 8");

            ImGui::Spacing();

            ImGui::Text("Current Combination");
            ImGui::BulletText("Particle");
            if (enableRing_) {
                ImGui::BulletText("Ring");
            }
            if (enableCylinder_) {
                ImGui::BulletText("Cylinder");
            }

            ImGui::EndTabItem();
        }

        // ==================================================
        // Post Effect
        // ==================================================
        if (ImGui::BeginTabItem("PostEffect")) {

            ImGui::Text("Post Effect Settings");
            ImGui::Separator();

            bool enableGrayScale = system_->GetDxCommon()->GetGrayScale();

            if (ImGui::Checkbox("GrayScale", &enableGrayScale)) {
                system_->GetDxCommon()->SetGrayScale(enableGrayScale);
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
        Camera* camera = system_->GetCamera();

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

    system_->GetImGuiManager()->End();
#endif
}
void GameScene::Draw() {
    Draw3D();
    Draw2D();
}

void GameScene::Draw3D() {
    DirectXCommon* dxCommon = system_->GetDxCommon();

    dxCommon->PreDrawForRenderTexture();

    system_->GetSrvManager()->PreDraw();

    system_->GetObject3dCommon()->PreDraw();

    for (auto& object : objects_) {
        object->Draw();
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

    system_->GetParticleManager()->Draw();
}

void GameScene::Draw2D() {
    DirectXCommon* dxCommon = system_->GetDxCommon();

    dxCommon->PreDraw();

    dxCommon->DrawRenderTextureToSwapChain();

    system_->GetSrvManager()->PreDraw();

    system_->GetSpriteCommon()->PreDraw();

    if (sprite_) {
        sprite_->Draw();
    }

    system_->GetImGuiManager()->Draw();

    dxCommon->PostDraw();
}