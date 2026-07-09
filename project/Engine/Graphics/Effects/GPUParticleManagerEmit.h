void GPUParticleManager::Emit(
    const Vector3& position,
    uint32_t count,
    float sizeMultiplier
) {
    if (!settings_.enabled)
    {
        return;
    }

    sizeMultiplier =
        (std::max)(sizeMultiplier, 0.01f);

    const int emitCount =
        std::clamp(
            settings_.fireCount,
            1,
            static_cast<int>(kMaxParticles)
        );

    emitterData_->translate = position;

    emitterData_->radius =
        settings_.spawnRadius *
        sizeMultiplier;

    emitterData_->count =
        static_cast<uint32_t>(emitCount);

    emitterData_->frequency = 0.5f;
    emitterData_->frequencyTime = 0.0f;
    emitterData_->emit = 1;
    emitterData_->effectType = 0.0f;

    emitterData_->sizeMultiplier =
        sizeMultiplier *
        (std::max)(settings_.particleScale, 0.01f);

    emitterData_->mainColor =
        settings_.fireMainColor;

    emitterData_->subColor =
        settings_.fireSubColor;

    emitRequested_ = true;

    // 既存APIとの互換性のため引数は残す
    (void)count;
}

void GPUParticleManager::EmitSakura(
    const Vector3& position,
    uint32_t count,
    float sizeMultiplier
) {
    if (!settings_.enabled)
    {
        return;
    }

    sizeMultiplier =
        (std::max)(sizeMultiplier, 0.01f);

    const int emitCount =
        std::clamp(
            settings_.sakuraCount,
            1,
            static_cast<int>(kMaxParticles)
        );

    emitterData_->translate = position;

    emitterData_->radius =
        settings_.spawnRadius *
        sizeMultiplier;

    emitterData_->count =
        static_cast<uint32_t>(emitCount);

    emitterData_->frequency = 0.5f;
    emitterData_->frequencyTime = 0.0f;
    emitterData_->emit = 1;
    emitterData_->effectType = 1.0f;

    emitterData_->sizeMultiplier =
        sizeMultiplier *
        (std::max)(settings_.particleScale, 0.01f);

    emitterData_->mainColor =
        settings_.sakuraMainColor;

    emitterData_->subColor =
        settings_.sakuraSubColor;

    emitRequested_ = true;

    (void)count;
}

