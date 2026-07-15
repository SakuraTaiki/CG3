#include "AnimationDebugPanel.h"
#include "AnimationDebugController.h"

#include <algorithm>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void AnimationDebugPanel::Draw(
    AnimationDebugController& animation
) {
#ifdef USE_IMGUI
    ImGui::SeparatorText(
        "Player Animation State"
    );

    ImGui::Text(
        "Current State : %s",
        animation.GetPlayerAnimationStateName()
    );

    const float movementHoldTime =
        animation.GetMovementHoldTime();

    ImGui::Text(
        "Movement Hold : %.2f / 2.00 sec",
        movementHoldTime
    );

    ImGui::ProgressBar(
        (std::min)(
            movementHoldTime / 2.0f,
            1.0f
            ),
        ImVec2(-1.0f, 0.0f)
    );

    ImGui::Text(
        "Animation Time : %.2f / %.2f",
        animation.GetAnimationTime(),
        animation.GetCurrentAnimationDuration()
    );

    ImGui::Text(
        "Playing : %s",
        animation.IsAnimationPlaying()
        ? "true"
        : "false"
    );

    ImGui::Text(
        "Loop : %s",
        animation.IsAnimationLoop()
        ? "true"
        : "false"
    );

    ImGui::Text(
        "Blending : %s",
        animation.IsAnimationBlending()
        ? "true"
        : "false"
    );

    bool manualTest =
        animation.IsManualAnimationTest();

    if (ImGui::Checkbox(
        "Manual Animation Test",
        &manualTest
    )) {
        animation.SetManualAnimationTest(
            manualTest
        );
    }

    if (manualTest) {
        if (ImGui::Button("Force Idle")) {
            animation.ForceIdleAnimation();
        }

        ImGui::SameLine();

        if (ImGui::Button("Force Walk")) {
            animation.ForceWalkAnimation();
        }

        ImGui::SameLine();

        if (ImGui::Button("Force Run")) {
            animation.ForceRunAnimation();
        }

        ImGui::SameLine();

        if (ImGui::Button("Force Slide")) {
            animation.ForceSlideAnimation();
        }

        ImGui::SameLine();

        if (ImGui::Button("Force Jump")) {
            animation.ForceJumpAnimation();
        }
    }
#else
    (void)animation;
#endif
}