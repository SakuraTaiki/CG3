#include "SoundController.h"
#include "Input.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void SoundController::Initialize() {
    sound_.Initialize();

    wavSoundData_ =
        sound_.SoundLoadFile("Resources/Sound/Alarm01.wav");

    mp4SoundData_ =
        sound_.SoundLoadFile("Resources/Sound/AlarmMovie.mp4");

    mp3SoundData_ =
        sound_.SoundLoadFile("Resources/Sound/maou_bgm_neorock83.mp3");
}

void SoundController::Finalize() {
    sound_.Finalize();
}

void SoundController::Update(Input* input) {
    if (!input) {
        return;
    }

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

void SoundController::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Text("Sound Volume");
    ImGui::Separator();

    ImGui::SliderFloat("Wav Volume", &wavVolume_, 0.0f, 1.0f);
    ImGui::SliderFloat("Mp4 Volume", &mp4Volume_, 0.0f, 1.0f);
    ImGui::SliderFloat("Mp3 Volume", &mp3Volume_, 0.0f, 1.0f);

    ImGui::Spacing();
    ImGui::TextDisabled("M key : Play mp4");
    ImGui::TextDisabled("N key : Play mp3");
    ImGui::TextDisabled("UP / DOWN : Change mp3 volume");
#endif
}