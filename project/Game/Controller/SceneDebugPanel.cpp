#include "SceneDebugPanel.h"

#include "EngineContext.h"
#include "HitEffectController.h"
#include "AnimationDebugController.h"
#include "SoundController.h"
#include "CameraDebugController.h"
#include "EnvironmentController.h"
#include "SceneObjectController.h"
#include "Object3dCommon.h"


#include "Ring.h"
#include "Cylinder.h"
#include "Primitive.h"
#include "ParticleManager.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "camera.h"
#include "GPUParticleManager.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void SceneDebugPanel::Draw(
    EngineContext* context,
    GameSceneDrawMode& drawMode,
    HitEffectController& hitEffect,
    AnimationDebugController& animationDebug,
    SoundController& soundController,
    CameraDebugController& cameraDebug,
    EnvironmentController& environment,
    SceneObjectController& sceneObjects,
    Ring* ring,
    Cylinder* cylinder,
    Primitive* primitive
) {
#ifdef USE_IMGUI
    if (!context) {
        return;
    }

    context->GetImGuiManager()->Begin();

    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin(
        "CG2 Effect Debug Panel",
        nullptr,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

    DrawSceneControl(drawMode, hitEffect, animationDebug);

    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Animation")) {
            animationDebug.DrawImGui();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Effect")) {
            DrawEffectTab(context, hitEffect, ring, cylinder, primitive);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("PostEffect")) {
            DrawPostEffectTab(context);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Environment")) {
            DrawEnvironmentTab(
                context->GetObject3dCommon(),
                environment,
                sceneObjects,
                animationDebug
            );

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sound")) {
            soundController.DrawImGui();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    cameraDebug.DrawImGui(context->GetCamera());

    ImGui::End();

    context->GetImGuiManager()->End();
#endif
}

void SceneDebugPanel::DrawSceneControl(
    GameSceneDrawMode& drawMode,
    HitEffectController& hitEffect,
    AnimationDebugController& animationDebug
) {
#ifdef USE_IMGUI
    ImGui::Text("Scene Control");
    ImGui::Separator();

    if (ImGui::Button("Emit Effect SPACE", ImVec2(180.0f, 32.0f))) {
        hitEffect.Emit({ 0.0f, 3.0f, 0.0f });
    }

    ImGui::SameLine();
    ImGui::TextDisabled("SPACE key also emits");

    ImGui::Spacing();

    ImGui::Text("Draw Mode");
    ImGui::Separator();

    int currentMode = static_cast<int>(drawMode);

    if (ImGui::RadioButton("Normal OBJ / Terrain", currentMode == 0)) {
        drawMode = GameSceneDrawMode::NormalObj;
    }

    if (ImGui::RadioButton("Animation glTF", currentMode == 1)) {
        drawMode = GameSceneDrawMode::Animation;
    }

    ImGui::Spacing();

    ImGui::Text("Skeleton");
    ImGui::BulletText("Joint Count : %d", animationDebug.GetJointCount());
    ImGui::BulletText("Animation Time : %.2f", animationDebug.GetAnimationTime());
    ImGui::Spacing();

    ImGui::Checkbox(
        "Show Skeleton Debug",
        &animationDebug.ShowSkeletonDebug()
    );
#endif
}


void SceneDebugPanel::DrawPostEffectTab(
    EngineContext* context
) {
#ifdef USE_IMGUI
    if (!context) {
        return;
    }

    DirectXCommon* dxCommon =
        context->GetDxCommon();

    if (!dxCommon) {
        return;
    }

    auto& vignette =
        dxCommon->GetVignetteSettings();

    auto& smoothing =
        dxCommon->GetSmoothingSettings();

    auto& gaussian =
        dxCommon->GetGaussianSettings();

    auto& radialBlur =
        dxCommon->GetRadialBlurSettings();

    auto& outline =
        dxCommon->GetOutlineSettings();

    auto& dissolve =
        dxCommon->GetDissolveSettings();

    auto& random =
        dxCommon->GetRandomSettings();

    Camera* camera =
        context->GetCamera();

    if (camera) {
        outline.nearClip =
            camera->GetNearClip();

        outline.farClip =
            camera->GetFarClip();
    }

    ImGui::Text("Post Effect Settings");
    ImGui::Separator();

    //====================
    // GrayScale
    //====================

    bool enableGrayScale =
        dxCommon->GetGrayScale();

    if (ImGui::Checkbox(
        "GrayScale",
        &enableGrayScale
    )) {
        dxCommon->SetGrayScale(
            enableGrayScale
        );

        if (enableGrayScale) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;
        }
    }

    //====================
    // Vignetting
    //====================

    ImGui::SeparatorText("Vignetting");

    if (ImGui::Checkbox(
        "Enable Vignetting",
        &vignette.enabled
    )) {
        if (vignette.enabled) {
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !vignette.enabled
    );

    ImGui::SliderFloat(
        "Vignette Intensity",
        &vignette.intensity,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Vignette Radius",
        &vignette.radius,
        0.0f,
        1.5f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Vignette Softness",
        &vignette.softness,
        0.01f,
        1.0f,
        "%.2f"
    );

    if (ImGui::Button(
        "Reset Vignetting"
    )) {
        const bool wasEnabled =
            vignette.enabled;

        vignette =
            DirectXCommon::VignetteSettings{};

        vignette.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Smoothing
    //====================

    ImGui::SeparatorText("Smoothing");

    if (ImGui::Checkbox(
        "Enable Smoothing",
        &smoothing.enabled
    )) {
        if (smoothing.enabled) {
            vignette.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !smoothing.enabled
    );

    ImGui::SliderInt(
        "Blur Radius",
        &smoothing.radius,
        1,
        4
    );

    ImGui::SliderFloat(
        "Blur Strength",
        &smoothing.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    const int smoothingKernelSize =
        smoothing.radius * 2 + 1;

    ImGui::Text(
        "Kernel : %d x %d",
        smoothingKernelSize,
        smoothingKernelSize
    );

    if (ImGui::Button(
        "Reset Smoothing"
    )) {
        const bool wasEnabled =
            smoothing.enabled;

        smoothing =
            DirectXCommon::SmoothingSettings{};

        smoothing.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Gaussian Filter
    //====================

    ImGui::SeparatorText(
        "Gaussian Filter"
    );

    if (ImGui::Checkbox(
        "Enable Gaussian Filter",
        &gaussian.enabled
    )) {
        if (gaussian.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !gaussian.enabled
    );

    ImGui::SliderInt(
        "Gaussian Radius",
        &gaussian.radius,
        1,
        4
    );

    ImGui::SliderFloat(
        "Gaussian Sigma",
        &gaussian.sigma,
        0.1f,
        10.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Gaussian Strength",
        &gaussian.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    const int gaussianKernelSize =
        gaussian.radius * 2 + 1;

    ImGui::Text(
        "Gaussian Kernel : %d x %d",
        gaussianKernelSize,
        gaussianKernelSize
    );

    if (ImGui::Button(
        "Reset Gaussian"
    )) {
        const bool wasEnabled =
            gaussian.enabled;

        gaussian =
            DirectXCommon::GaussianSettings{};

        gaussian.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Radial Blur
    //====================

    ImGui::SeparatorText("Radial Blur");

    if (ImGui::Checkbox(
        "Enable Radial Blur",
        &radialBlur.enabled
    )) {
        if (radialBlur.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !radialBlur.enabled
    );

    ImGui::SliderFloat2(
        "Blur Center (UV)",
        radialBlur.center,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Radial Blur Width",
        &radialBlur.blurWidth,
        0.0f,
        0.1f,
        "%.4f"
    );

    ImGui::SliderFloat(
        "Radial Blur Strength",
        &radialBlur.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderInt(
        "Radial Samples",
        &radialBlur.sampleCount,
        2,
        32
    );

    ImGui::TextDisabled(
        "Center: (0,0) top-left / "
        "(1,1) bottom-right"
    );

    if (ImGui::Button(
        "Reset Radial Blur"
    )) {
        const bool wasEnabled =
            radialBlur.enabled;

        radialBlur =
            DirectXCommon::RadialBlurSettings{};

        radialBlur.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Depth Based Outline
    //====================

    ImGui::SeparatorText(
        "Depth Based Outline"
    );

    if (ImGui::Checkbox(
        "Enable Depth Outline",
        &outline.enabled
    )) {
        if (outline.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !outline.enabled
    );

    ImGui::ColorEdit4(
        "Outline Color",
        outline.color
    );

    ImGui::SliderFloat(
        "Outline Threshold",
        &outline.threshold,
        0.001f,
        1.0f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Outline Strength",
        &outline.strength,
        0.0f,
        5.0f,
        "%.2f"
    );

    ImGui::SliderInt(
        "Outline Thickness",
        &outline.thickness,
        1,
        4
    );

    ImGui::Text(
        "Near Clip : %.3f",
        outline.nearClip
    );

    ImGui::Text(
        "Far Clip : %.3f",
        outline.farClip
    );

    if (ImGui::Button(
        "Reset Outline"
    )) {
        const bool wasEnabled =
            outline.enabled;

        outline =
            DirectXCommon::OutlineSettings{};

        outline.enabled =
            wasEnabled;

        if (camera) {
            outline.nearClip =
                camera->GetNearClip();

            outline.farClip =
                camera->GetFarClip();
        }
    }

    ImGui::EndDisabled();


    //====================
    // Dissolve
    //====================

    ImGui::SeparatorText("Dissolve");

    if (ImGui::Checkbox(
        "Enable Dissolve",
        &dissolve.enabled
    )) {
        if (dissolve.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            // RadialBlur追加済みの場合
            radialBlur.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !dissolve.enabled
    );

    ImGui::SliderFloat(
        "Dissolve Threshold",
        &dissolve.threshold,
        0.0f,
        1.0f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Dissolve Edge Width",
        &dissolve.edgeWidth,
        0.001f,
        0.25f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Dissolve Edge Intensity",
        &dissolve.edgeIntensity,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::ColorEdit4(
        "Dissolve Edge Color",
        dissolve.edgeColor
    );

    if (ImGui::Button(
        "Reset Dissolve"
    )) {
        const bool wasEnabled =
            dissolve.enabled;

        dissolve =
            DirectXCommon::DissolveSettings{};

        dissolve.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();


    //====================
    //RandomNoise
    //====================

    ImGui::SeparatorText("Random Noise");

    if (ImGui::Checkbox(
        "Enable Random Noise",
        &random.enabled
    )) {
        if (random.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            // 追加済みの場合
            radialBlur.enabled = false;
            dissolve.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !random.enabled
    );

    ImGui::Checkbox(
        "Animate Random Noise",
        &random.animate
    );

    ImGui::Checkbox(
        "Show Noise Only",
        &random.showNoiseOnly
    );

    ImGui::SliderFloat(
        "Random Speed",
        &random.speed,
        0.0f,
        10.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Random Scale",
        &random.scale,
        1.0f,
        2000.0f,
        "%.0f"
    );

    ImGui::SliderFloat(
        "Random Strength",
        &random.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    if (ImGui::Button(
        "Reset Random Settings"
    )) {
        const bool wasEnabled =
            random.enabled;

        random =
            DirectXCommon::RandomSettings{};

        random.enabled =
            wasEnabled;

        dxCommon->ResetRandomTime();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Reset Random Time"
    )) {
        dxCommon->ResetRandomTime();
    }

    ImGui::EndDisabled();

    //====================
    // Current Effect
    //====================

    ImGui::SeparatorText(
        "Current Post Effect"
    );

    const char* currentEffect = "None";

    if (outline.enabled) {
        currentEffect =
            "Depth Based Outline";
    } else if (random.enabled) {
        currentEffect =
            "Random Noise";
    } else if (dissolve.enabled) {
        currentEffect =
            "Dissolve";
    } else if (radialBlur.enabled) {
        currentEffect =
            "Radial Blur";
    } else if (gaussian.enabled) {
        currentEffect =
            "Gaussian Filter";
    } else if (smoothing.enabled) {
        currentEffect =
            "Smoothing";
    } else if (vignette.enabled) {
        currentEffect =
            "Vignetting";
    } else if (dxCommon->GetGrayScale()) {
        currentEffect =
            "GrayScale";
    }

    ImGui::Text(
        "Active : %s",
        currentEffect
    );

    if (ImGui::Button(
        "Disable All Post Effects"
    )) {
        vignette.enabled = false;
        smoothing.enabled = false;
        gaussian.enabled = false;
        radialBlur.enabled = false;
        outline.enabled = false;
        dissolve.enabled = false;
        random.enabled = false;

        dxCommon->SetGrayScale(false);
    }
#endif
}


void SceneDebugPanel::DrawEnvironmentTab(
    Object3dCommon* object3dCommon,
    EnvironmentController& environment,
    SceneObjectController& sceneObjects,
    AnimationDebugController& animationDebug
) {
#ifdef USE_IMGUI
    ImGui::Checkbox(
        "Show Terrain",
        &sceneObjects.ShowTerrain()
    );

    ImGui::Checkbox(
        "Show Axis",
        &sceneObjects.ShowAxis()
    );

    if (environment.DrawImGui()) {
        sceneObjects.SetEnvironmentCoefficient(
            environment.GetEnvironmentCoefficient()
        );

        animationDebug.SetEnvironmentCoefficient(
            environment.GetEnvironmentCoefficient()
        );
    }

    if (!object3dCommon) {
        return;
    }


    ImGui::SeparatorText("Light Settings");

    // ==================================================
    // DirectionalLight
    // ==================================================

    if (ImGui::CollapsingHeader(
        "Directional Light",
        ImGuiTreeNodeFlags_DefaultOpen
    )) {
        DirectionalLight& light =
            object3dCommon->GetDirectionalLight();

        ImGui::ColorEdit4(
            "Directional Color",
            &light.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        if (ImGui::DragFloat3(
            "Directional Direction",
            &light.direction.x,
            0.01f,
            -1.0f,
            1.0f
        )) {
            float length = std::sqrt(
                light.direction.x * light.direction.x +
                light.direction.y * light.direction.y +
                light.direction.z * light.direction.z
            );

            if (length > 0.0001f) {
                light.direction.x /= length;
                light.direction.y /= length;
                light.direction.z /= length;
            }
        }

        ImGui::DragFloat(
            "Directional Intensity",
            &light.intensity,
            0.01f,
            0.0f,
            20.0f
        );

        if (ImGui::Button("Reset Directional Light")) {
            light.color =
            { 1.0f, 1.0f, 1.0f, 1.0f };

            light.direction =
            { 0.0f, -1.0f, 0.0f };

            light.intensity = 0.2f;
        }
    }

    // ==================================================
    // SpotLight
    // ==================================================

    if (ImGui::CollapsingHeader(
        "Spot Light",
        ImGuiTreeNodeFlags_DefaultOpen
    )) {
        SpotLight& light =
            object3dCommon->GetSpotLight();

        ImGui::ColorEdit4(
            "Spot Color",
            &light.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragFloat3(
            "Spot Position",
            &light.position.x,
            0.05f,
            -100.0f,
            100.0f
        );

        if (ImGui::DragFloat3(
            "Spot Direction",
            &light.direction.x,
            0.01f,
            -1.0f,
            1.0f
        )) {
            float length = std::sqrt(
                light.direction.x * light.direction.x +
                light.direction.y * light.direction.y +
                light.direction.z * light.direction.z
            );

            if (length > 0.0001f) {
                light.direction.x /= length;
                light.direction.y /= length;
                light.direction.z /= length;
            }
        }

        ImGui::DragFloat(
            "Spot Intensity",
            &light.intensity,
            0.05f,
            0.0f,
            100.0f
        );

        ImGui::DragFloat(
            "Spot Distance",
            &light.distance,
            0.1f,
            0.1f,
            200.0f
        );

        ImGui::DragFloat(
            "Spot Decay",
            &light.decay,
            0.01f,
            0.01f,
            10.0f
        );

        constexpr float kPi =
            3.14159265358979323846f;

        // shaderにはcos値を渡しているため、
        // ImGuiでは扱いやすい角度へ一度戻す
        float outerAngle =
            std::acos(std::clamp(
                light.cosAngle,
                -1.0f,
                1.0f
            )) * 180.0f / kPi;

        float innerAngle =
            std::acos(std::clamp(
                light.cosFalloffStart,
                -1.0f,
                1.0f
            )) * 180.0f / kPi;

        bool angleChanged = false;

        angleChanged |= ImGui::SliderFloat(
            "Spot Inner Angle",
            &innerAngle,
            0.0f,
            89.0f,
            "%.1f deg"
        );

        angleChanged |= ImGui::SliderFloat(
            "Spot Outer Angle",
            &outerAngle,
            1.0f,
            89.0f,
            "%.1f deg"
        );

        if (angleChanged) {
            // 内側角度が外側を超えないようにする
            innerAngle = std::clamp(
                innerAngle,
                0.0f,
                outerAngle
            );

            outerAngle = std::clamp(
                outerAngle,
                innerAngle,
                89.0f
            );

            light.cosAngle =
                std::cos(
                    outerAngle * kPi / 180.0f
                );

            light.cosFalloffStart =
                std::cos(
                    innerAngle * kPi / 180.0f
                );
        }

        ImGui::Spacing();

        ImGui::TextDisabled(
            "Inner: %.1f deg / Outer: %.1f deg",
            innerAngle,
            outerAngle
        );

        ImGui::TextDisabled(
            "Cos Inner: %.3f / Cos Outer: %.3f",
            light.cosFalloffStart,
            light.cosAngle
        );

        if (ImGui::Button("Reset Spot Light")) {
            light.color =
            { 1.0f, 1.0f, 1.0f, 1.0f };

            light.position =
            { 0.0f, 5.0f, 0.0f };

            light.direction =
            { 0.0f, -1.0f, 0.0f };

            light.intensity = 4.0f;
            light.distance = 15.0f;
            light.decay = 2.0f;

            light.cosAngle =
                std::cos(
                    30.0f * kPi / 180.0f
                );

            light.cosFalloffStart =
                std::cos(
                    20.0f * kPi / 180.0f
                );

            light.padding = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Turn Off Spot Light")) {
            light.intensity = 0.0f;
        }
    }


#endif
}

void SceneDebugPanel::DrawEffectTab(
    EngineContext* context,
    HitEffectController& hitEffect,
    Ring* ring,
    Cylinder* cylinder,
    Primitive* primitive
) {



#ifdef USE_IMGUI
    ImGui::Checkbox(
        "Enable Primitive",
        &hitEffect.EnablePrimitive()
    );

    ImGui::Checkbox(
        "Enable Ring",
        &hitEffect.EnableRing()
    );

    ImGui::Checkbox(
        "Enable Cylinder",
        &hitEffect.EnableCylinder()
    );

    GPUParticleManager* gpuParticleManager =
        context->GetGPUParticleManager();

    if (gpuParticleManager)
    {
        GPUParticleManager::Settings& settings =
            gpuParticleManager->GetSettings();

        ImGui::SeparatorText("GPU Particle");

        ImGui::Checkbox(
            "Enable GPU Particle",
            &settings.enabled
        );

        ImGui::DragInt(
            "GPU Fire Count",
            &settings.fireCount,
            1.0f,
            1,
            static_cast<int>(
                GPUParticleManager::kMaxParticles
                )
        );

        ImGui::DragInt(
            "GPU Sakura Count",
            &settings.sakuraCount,
            1.0f,
            1,
            static_cast<int>(
                GPUParticleManager::kMaxParticles
                )
        );

        ImGui::DragFloat(
            "GPU Particle Scale",
            &settings.particleScale,
            0.01f,
            0.01f,
            10.0f,
            "%.2f"
        );

        ImGui::DragFloat(
            "GPU Spawn Radius",
            &settings.spawnRadius,
            0.01f,
            0.0f,
            20.0f,
            "%.2f"
        );

        ImGui::ColorEdit4(
            "GPU Fire Main Color",
            &settings.fireMainColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "GPU Fire Sub Color",
            &settings.fireSubColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "GPU Sakura Main Color",
            &settings.sakuraMainColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "GPU Sakura Sub Color",
            &settings.sakuraSubColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        if (ImGui::Button("Reset GPU Particle"))
        {
            settings =
                GPUParticleManager::Settings{};
        }

        ImGui::SameLine();


        ImGui::Text(
            "GPU Particle : %s",
            settings.enabled ? "ON" : "OFF"
        );

        ImGui::Text(
            "Emit Position : %.2f, %.2f, %.2f",
            hitEffect.GetPosition().x,
            hitEffect.GetPosition().y,
            hitEffect.GetPosition().z
        );

        ImGui::Text(
            "Fire Count : %d",
            settings.fireCount
        );
        
        if (ImGui::Button("Preview GPU Particle"))
        {
            // OFFだった場合もPreviewではONにする
            settings.enabled = true;

            const Vector3 emitPosition =
                hitEffect.GetPosition();

            const float emitSize =
                (std::max)(
                    hitEffect.GetSize(),
                    0.01f
                    );

            if (
                hitEffect.GetType() ==
                HitEffectController::Type::Sakura
                ) {
                settings.sakuraCount =
                    std::clamp(
                        settings.sakuraCount,
                        1,
                        static_cast<int>(
                            GPUParticleManager::kMaxParticles
                            )
                    );

                gpuParticleManager->EmitSakura(
                    emitPosition,
                    static_cast<uint32_t>(
                        settings.sakuraCount
                        ),
                    emitSize
                );
            } else
            {
                settings.fireCount =
                    std::clamp(
                        settings.fireCount,
                        1,
                        static_cast<int>(
                            GPUParticleManager::kMaxParticles
                            )
                    );

                gpuParticleManager->Emit(
                    emitPosition,
                    static_cast<uint32_t>(
                        settings.fireCount
                        ),
                    emitSize
                );
            }
        }
    }

    ImGui::SeparatorText("Hit Effect Type");

    if (ImGui::RadioButton(
        "Fire",
        hitEffect.GetType() == HitEffectController::Type::Fire
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;
    }

    ImGui::SameLine();

    if (ImGui::RadioButton(
        "Sakura",
        hitEffect.GetType() == HitEffectController::Type::Sakura
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Sakura;
    }

    ImGui::SliderFloat(
        "Hit Effect Size",
        &hitEffect.GetSize(),
        0.1f,
        5.0f,
        "%.2f"
    );

    if (
        hitEffect.GetType() ==
        HitEffectController::Type::Sakura
        ) {
        ParticleManager::SakuraSettings& settings =
            context
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

        if (ImGui::Button("Reset Sakura Settings")) {
            settings =
                ParticleManager::SakuraSettings{};
        }
    }

    ImGui::SeparatorText("Emit");

    ImGui::DragFloat3(
        "Effect Position",
        &hitEffect.GetPosition().x,
        0.05f,
        -20.0f,
        20.0f
    );

    ImGui::SeparatorText("Hit Effect Size");

    ImGui::SliderFloat(
        "Effect Size",
        &hitEffect.GetSize(),
        0.1f,
        5.0f,
        "%.2f"
    );

    if (ImGui::Button("Reset Effect Size")) {
        hitEffect.GetSize() = 1.0f;
    }

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Current : %.2f x",
        hitEffect.GetSize()
    );

    ImGui::SeparatorText("Effect Presets");

    ImGui::InputText(
        "Preset Name",
        hitEffect.GetPresetNameBuffer().data(),
        hitEffect.GetPresetNameBuffer().size()
    );

    if (ImGui::Button(
        "Save Current Preset",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.SavePreset(
            hitEffect.GetPresetNameBuffer().data()
        );
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Refresh Presets",
        ImVec2(160.0f, 30.0f)
    )) {
        hitEffect.RefreshPresetList();
    }

    if (!hitEffect.GetPresetNames().empty()) {
        hitEffect.GetSelectedPreset() =
            std::clamp(
                hitEffect.GetSelectedPreset(),
                0,
                static_cast<int>(
                    hitEffect.GetPresetNames().size()
                    ) - 1
            );

        const char* previewName =
            hitEffect.GetPresetNames()[
                hitEffect.GetSelectedPreset()
            ].c_str();

        if (ImGui::BeginCombo(
            "Saved Presets",
            previewName
        )) {
            for (
                int index = 0;
                index <
                static_cast<int>(
                    hitEffect.GetPresetNames().size()
                    );
                    ++index
                ) {
                const bool selected =
                    hitEffect.GetSelectedPreset() ==
                    index;

                if (ImGui::Selectable(
                    hitEffect.GetPresetNames()[index].c_str(),
                    selected
                )) {
                    hitEffect.GetSelectedPreset() =
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
            hitEffect.LoadPreset(
                hitEffect.GetPresetNames()[
                    hitEffect.GetSelectedPreset()
                ]
            );
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Load And Preview",
            ImVec2(180.0f, 30.0f)
        )) {
            if (hitEffect.LoadPreset(
                hitEffect.GetPresetNames()[
                    hitEffect.GetSelectedPreset()
                ]
            )) {
                hitEffect.Emit(
                    hitEffect.GetPosition()
                );
            }
        }
    } else {
        ImGui::TextDisabled(
            "No saved presets"
        );
    }

    if (!hitEffect.GetMessage().empty()) {
        ImGui::TextDisabled(
            "%s",
            hitEffect.GetMessage().c_str()
        );
    }

    ImGui::SeparatorText("Preview");

    if (ImGui::Button(
        "Preview Current",
        ImVec2(180.0f, 32.0f)
    )) {
        hitEffect.Emit(
            hitEffect.GetPosition()
        );
    }

    if (ImGui::Button(
        "Apply Fire Preset",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;

        hitEffect.ApplyFirePreset();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Apply Fire And Preview",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;

        hitEffect.ApplyFirePreset();

        hitEffect.Emit(
            hitEffect.GetPosition()
        );
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Preview Fire",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.Emit(hitEffect.GetPosition());
    }

    if (ring) {
        Ring::Settings& settings =
            ring->GetSettings();

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

    if (primitive) {
        Primitive::Settings& settings =
            primitive->GetSettings();

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

    if (cylinder) {
        Cylinder::Settings& settings =
            cylinder->GetSettings();

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

#endif
}