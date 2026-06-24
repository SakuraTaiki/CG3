#include "EnvironmentController.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void EnvironmentController::Initialize(
    DirectXCommon* dxCommon,
    TextureManager* textureManager
) {
    skybox_ = std::make_unique<Skybox>();

    skybox_->Initialize(
        dxCommon,
        textureManager,
        "Resources/skybox/rostock_laage_airport_4k.dds"
    );

    skybox_->SetScale({ 100.0f, 100.0f, 100.0f });

    environmentTextureHandle_ =
        textureManager->LoadTexture(
            "Resources/skybox/rostock_laage_airport_4k.dds"
        );
}

void EnvironmentController::Finalize() {
    skybox_.reset();
}

void EnvironmentController::Update(
    const Matrix4x4& view,
    const Matrix4x4& projection
) {
    if (!enableSkybox_ || !skybox_) {
        return;
    }

    skybox_->SetCamera(
        view,
        projection
    );

    skybox_->Update();
}

void EnvironmentController::Draw() {
    if (!enableSkybox_ || !skybox_) {
        return;
    }

    skybox_->Draw();
}

bool EnvironmentController::DrawImGui() {
#ifdef USE_IMGUI
    bool coefficientChanged = false;

    ImGui::Checkbox(
        "Enable Skybox",
        &enableSkybox_
    );

    ImGui::Text("Environment Lighting");
    ImGui::Separator();

    coefficientChanged =
        ImGui::SliderFloat(
            "Environment Coefficient",
            &environmentCoefficient_,
            0.0f,
            1.0f
        );

    return coefficientChanged;
#else
    return false;
#endif
}