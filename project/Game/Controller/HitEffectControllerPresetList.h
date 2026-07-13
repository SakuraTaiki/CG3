void HitEffectController::RefreshPresetList() {
    namespace fs = std::filesystem;

    const fs::path directory =
        "Resources/Settings/HitEffects";

    fs::create_directories(directory);

    presetNames_.clear();

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

        presetNames_.push_back(
            entry.path().stem().string()
        );
    }

    std::sort(
        presetNames_.begin(),
        presetNames_.end()
    );

    if (presetNames_.empty()) {
        selectedPreset_ = 0;
    } else {
        selectedPreset_ =
            std::clamp(
                selectedPreset_,
                0,
                static_cast<int>(presetNames_.size()) - 1
            );
    }
}
