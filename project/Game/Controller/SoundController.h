#pragma once

#include "Sound.h"

class Input;

class SoundController {
public:
    void Initialize();
    void Finalize();

    void Update(Input* input);
    void DrawImGui();

private:
    Sound sound_;

    Sound::SoundData wavSoundData_{};
    Sound::SoundData mp4SoundData_{};
    Sound::SoundData mp3SoundData_{};

    float wavVolume_ = 0.5f;
    float mp4Volume_ = 0.5f;
    float mp3Volume_ = 0.5f;
};