#include "GameScene.h"
#include "ModelManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void GameScene::Initialize(GameSystem* system) {
    system_ = system;

    ModelManager::Initialize(system_->GetObject3dCommon());

    InitializeModels();
    InitializeObjects();
    InitializeSprite();
    InitializeSkybox();
    InitializeSound();
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
    ModelManager::Load("plane.obj");
    ModelManager::Load("axis.obj");
}

void GameScene::InitializeObjects() {
    Object3dCommon* object3dCommon = system_->GetObject3dCommon();

    Model* modelPlane = ModelManager::Load("plane.obj");
    Model* modelAxis = ModelManager::Load("axis.obj");

    {
        std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
        object->Initialize(object3dCommon);
        object->SetModel(modelPlane);
        object->SetPosition({ 0.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });
        object->SetScale({ 10.0f, 1.0f, 10.0f });
        objects_.push_back(std::move(object));
    }

    {
        std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
        object->Initialize(object3dCommon);
        object->SetModel(modelAxis);
        object->SetPosition({ 2.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });
        objects_.push_back(std::move(object));
    }

    {
        std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
        object->Initialize(object3dCommon);
        object->SetModel(modelAxis);
        object->SetPosition({ -2.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });
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
}

void GameScene::InitializeSound() {
    sound_.Initialize();

    wavSoundData_ = sound_.SoundLoadFile("Resources/Sound/Alarm01.wav");
    mp4SoundData_ = sound_.SoundLoadFile("Resources/Sound/AlarmMovie.mp4");
    mp3SoundData_ = sound_.SoundLoadFile("Resources/Sound/maou_bgm_neorock83.mp3");
}

void GameScene::Update() {
    Input* input = system_->GetInput();
    input->Update();

    if (input->TriggerKey(DIK_SPACE)) {
        system_->GetParticleManager()->Emit({ 0.0f, 0.0f, 0.0f }, 10);
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

    ImGui::Begin("Sound");
    ImGui::SliderFloat("wavVolume", &wavVolume_, 0.0f, 1.0f);
    ImGui::SliderFloat("mp4Volume", &mp4Volume_, 0.0f, 1.0f);
    ImGui::SliderFloat("mp3Volume", &mp3Volume_, 0.0f, 1.0f);
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