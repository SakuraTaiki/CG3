#include "HitEffectController.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

void HitEffectController::Initialize(
    Primitive* primitive,
    Ring* ring,
    Cylinder* cylinder,
    ParticleManager* particleManager
) {
    primitive_ = primitive;
    ring_ = ring;
    cylinder_ = cylinder;
    particleManager_ = particleManager;
}

std::string HitEffectController::MakeSafePresetName(
    const std::string& name
) const {
    std::string safeName;

    for (unsigned char character : name) {
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

    return safeName;
}

void HitEffectController::UpdateActiveFlags() {
    if (primitive_) {
        primitive_->SetIsActive(enablePrimitive_);
    }

    if (ring_) {
        ring_->SetIsActive(enableRing_);
    }

    if (cylinder_) {
        cylinder_->SetIsActive(enableCylinder_);
    }
}

void HitEffectController::Emit(const Vector3& position) {
    const float effectSize =
        (std::max)(size_, 0.01f);

    const auto emitComponents = [&]() {
        if (enablePrimitive_ && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();

            const float originalWidth = settings.width;
            const float originalMinLength = settings.minLength;
            const float originalMaxLength = settings.maxLength;
            const float originalMoveSpeed = settings.moveSpeed;

            settings.width = originalWidth * effectSize;
            settings.minLength = originalMinLength * effectSize;
            settings.maxLength = originalMaxLength * effectSize;
            settings.moveSpeed = originalMoveSpeed * effectSize;

            primitive_->Emit(position);

            settings.width = originalWidth;
            settings.minLength = originalMinLength;
            settings.maxLength = originalMaxLength;
            settings.moveSpeed = originalMoveSpeed;
        }

        if (enableRing_ && ring_) {
            Ring::Settings& settings =
                ring_->GetSettings();

            const float originalStartScale = settings.startScale;
            const float originalEndScale = settings.endScale;

            settings.startScale = originalStartScale * effectSize;
            settings.endScale = originalEndScale * effectSize;

            ring_->Emit(position);

            settings.startScale = originalStartScale;
            settings.endScale = originalEndScale;
        }

        if (enableCylinder_ && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();

            const float originalRadius = settings.radius;
            const float originalStartHeight = settings.startHeight;
            const float originalEndHeight = settings.endHeight;
            const float originalRiseDistance = settings.riseDistance;
            const Vector3 originalPositionOffset = settings.positionOffset;

            settings.radius = originalRadius * effectSize;
            settings.startHeight = originalStartHeight * effectSize;
            settings.endHeight = originalEndHeight * effectSize;
            settings.riseDistance = originalRiseDistance * effectSize;
            settings.positionOffset = {
                originalPositionOffset.x * effectSize,
                originalPositionOffset.y * effectSize,
                originalPositionOffset.z * effectSize
            };

            cylinder_->Emit(position);

            settings.radius = originalRadius;
            settings.startHeight = originalStartHeight;
            settings.endHeight = originalEndHeight;
            settings.riseDistance = originalRiseDistance;
            settings.positionOffset = originalPositionOffset;
        }
        };

    if (type_ == Type::Sakura) {
        if (particleManager_) {
            particleManager_->EmitSakura(position, effectSize);
        }

        emitComponents();
        return;
    }

    if (particleManager_) {
        particleManager_->Emit(position, 28);
    }

    emitComponents();
}

void HitEffectController::ApplyFirePreset() {
    if (primitive_) {
        Primitive::Settings& settings =
            primitive_->GetSettings();

        settings = Primitive::Settings{};

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

    if (ring_) {
        Ring::Settings& settings =
            ring_->GetSettings();

        settings = Ring::Settings{};

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

    if (cylinder_) {
        Cylinder::Settings& settings =
            cylinder_->GetSettings();

        settings = Cylinder::Settings{};

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

bool HitEffectController::SavePreset(
    const std::string& presetName
) {
    namespace fs = std::filesystem;

    const std::string safeName =
        MakeSafePresetName(presetName);

    const fs::path directory =
        "Resources/Settings/HitEffects";

    fs::create_directories(directory);

    const fs::path filePath =
        directory / (safeName + ".txt");

    std::ofstream file(filePath);

    if (!file.is_open()) {
        message_ = "Preset save failed";
        return false;
    }

    const auto writeVector3 =
        [&](const char* name, const Vector3& value) {
        file << name << " "
            << value.x << " "
            << value.y << " "
            << value.z << "\n";
        };

    const auto writeVector4 =
        [&](const char* name, const Vector4& value) {
        file << name << " "
            << value.x << " "
            << value.y << " "
            << value.z << " "
            << value.w << "\n";
        };

    file << "PresetVersion 2\n";
    writeVector3("EffectPosition", position_);
    file << "EffectSize " << size_ << "\n";
    file << "HitEffectType " << static_cast<int>(type_) << "\n";
    file << "EnablePrimitive " << enablePrimitive_ << "\n";
    file << "EnableRing " << enableRing_ << "\n";
    file << "EnableCylinder " << enableCylinder_ << "\n";

    if (primitive_) {
        const Primitive::Settings& settings =
            primitive_->GetSettings();

        writeVector4("PrimitiveColor", settings.color);
        writeVector4("PrimitiveEndColor", settings.endColor);
        file << "PrimitiveCount " << settings.count << "\n";
        file << "PrimitiveIntensity " << settings.intensity << "\n";
        file << "PrimitiveWidth " << settings.width << "\n";
        file << "PrimitiveWidthRandomness " << settings.widthRandomness << "\n";
        file << "PrimitiveLength " << settings.minLength << " " << settings.maxLength << "\n";
        file << "PrimitiveLifeTime " << settings.minLifeTime << " " << settings.maxLifeTime << "\n";
        file << "PrimitiveMoveSpeed " << settings.moveSpeed << "\n";
        file << "PrimitiveMoveSpeedRandomness " << settings.moveSpeedRandomness << "\n";
        file << "PrimitiveRotationSpeed " << settings.rotationSpeed << "\n";
        file << "PrimitiveRotationSpeedRandomness " << settings.rotationSpeedRandomness << "\n";
        file << "PrimitiveDirection " << settings.directionAngle << " " << settings.directionSpread << "\n";
        file << "PrimitiveSpawnRadius " << settings.spawnRadius << "\n";
        writeVector3("PrimitiveAcceleration", settings.acceleration);
        file << "PrimitiveEndScale " << settings.endWidthScale << " " << settings.endLengthScale << "\n";
        file << "PrimitiveScaleEasePower " << settings.scaleEasePower << "\n";
        file << "PrimitiveFadeInRatio " << settings.fadeInRatio << "\n";
        file << "PrimitiveFadePower " << settings.fadePower << "\n";
    }

    if (ring_) {
        const Ring::Settings& settings =
            ring_->GetSettings();

        writeVector4("RingColor", settings.color);
        writeVector4("RingEndColor", settings.endColor);
        file << "RingIntensity " << settings.intensity << "\n";
        file << "RingScale " << settings.startScale << " " << settings.endScale << "\n";
        file << "RingThickness " << settings.thickness << "\n";
        file << "RingEndThickness " << settings.endThickness << "\n";
        file << "RingLifeTime " << settings.lifeTime << "\n";
        file << "RingFadeInRatio " << settings.fadeInRatio << "\n";
        file << "RingEasePower " << settings.easePower << "\n";
        file << "RingRotationSpeed " << settings.rotationSpeed << "\n";
        file << "RingEdgeSoftness " << settings.edgeSoftness << "\n";
        file << "RingGlowStrength " << settings.glowStrength << "\n";
        file << "RingDistortion "
            << settings.distortionStrength << " "
            << settings.distortionFrequency << " "
            << settings.distortionSpeed << "\n";
    }

    if (cylinder_) {
        const Cylinder::Settings& settings =
            cylinder_->GetSettings();

        writeVector4("CylinderColor", settings.color);
        writeVector4("CylinderEndColor", settings.endColor);
        file << "CylinderIntensity " << settings.intensity << "\n";
        file << "CylinderRadius " << settings.radius << "\n";
        file << "CylinderEndRadius " << settings.endRadius << "\n";
        file << "CylinderHeight " << settings.startHeight << " " << settings.endHeight << "\n";
        file << "CylinderRadiusScale " << settings.bottomRadiusScale << " " << settings.topRadiusScale << "\n";
        file << "CylinderLifeTime " << settings.lifeTime << "\n";
        file << "CylinderRiseDistance " << settings.riseDistance << "\n";
        file << "CylinderEasePower " << settings.easePower << "\n";
        file << "CylinderFadeInRatio " << settings.fadeInRatio << "\n";
        file << "CylinderFadePower " << settings.fadePower << "\n";
        file << "CylinderTwist " << settings.twistAmount << " " << settings.twistSpeed << "\n";
        file << "CylinderNoise "
            << settings.noiseStrength << " "
            << settings.noiseFrequency << " "
            << settings.noiseSpeed << "\n";
        file << "CylinderVerticalFade " << settings.topFade << " " << settings.bottomFade << "\n";
        writeVector3("CylinderPositionOffset", settings.positionOffset);
    }

    if (particleManager_) {
        const ParticleManager::SakuraSettings& settings =
            particleManager_->GetSakuraSettings();

        writeVector4("SakuraColor", settings.color);
        writeVector4("SakuraSubColor", settings.subColor);
        file << "SakuraPetalCount " << settings.petalCount << "\n";
        file << "SakuraSize " << settings.minSize << " " << settings.maxSize << "\n";
        file << "SakuraSpawnRadius " << settings.spawnRadius << "\n";
        file << "SakuraSpeed " << settings.spreadSpeed << " " << settings.upwardSpeed << "\n";
        file << "SakuraLifeTime " << settings.minLifeTime << " " << settings.maxLifeTime << "\n";
        file << "SakuraGravity " << settings.gravity << "\n";
        file << "SakuraRotationSpeed " << settings.rotationSpeed << "\n";
        file << "SakuraFlash " << settings.flashSize << " " << settings.flashLifeTime << "\n";
    }

    message_ = "Saved : " + safeName;
    RefreshPresetList();

    return true;
}


bool HitEffectController::LoadPreset(
    const std::string& presetName
) {
    namespace fs = std::filesystem;

    const std::string safeName =
        MakeSafePresetName(presetName);

    const fs::path filePath =
        fs::path("Resources/Settings/HitEffects") /
        (safeName + ".txt");

    std::ifstream file(filePath);

    if (!file.is_open()) {
        message_ = "Preset not found : " + safeName;
        return false;
    }

    type_ = Type::Fire;

    if (primitive_) {
        primitive_->GetSettings() = Primitive::Settings{};
    }

    if (ring_) {
        ring_->GetSettings() = Ring::Settings{};
    }

    if (cylinder_) {
        cylinder_->GetSettings() = Cylinder::Settings{};
    }

    if (particleManager_) {
        particleManager_->GetSakuraSettings() =
            ParticleManager::SakuraSettings{};
    }

    const auto readVector3 =
        [&](Vector3& value) {
        file >> value.x >> value.y >> value.z;
        };

    const auto readVector4 =
        [&](Vector4& value) {
        file >> value.x >> value.y >> value.z >> value.w;
        };

    int presetVersion = 1;
    std::string key;

    while (file >> key) {
        if (key == "PresetVersion") {
            file >> presetVersion;
        } else if (key == "EffectPosition") {
            readVector3(position_);
        } else if (key == "EffectSize") {
            file >> size_;
        } else if (key == "HitEffectType") {
            int effectType = 0;
            file >> effectType;

            type_ =
                effectType == static_cast<int>(Type::Sakura)
                ? Type::Sakura
                : Type::Fire;
        } else if (key == "EnablePrimitive") {
            file >> enablePrimitive_;
        } else if (key == "EnableRing") {
            file >> enableRing_;
        } else if (key == "EnableCylinder") {
            file >> enableCylinder_;
        } else if (key == "PrimitiveColor" && primitive_) {
            readVector4(primitive_->GetSettings().color);
        } else if (key == "PrimitiveEndColor" && primitive_) {
            readVector4(primitive_->GetSettings().endColor);
        } else if (key == "PrimitiveCount" && primitive_) {
            file >> primitive_->GetSettings().count;
        } else if (key == "PrimitiveIntensity" && primitive_) {
            file >> primitive_->GetSettings().intensity;
        } else if (key == "PrimitiveWidth" && primitive_) {
            file >> primitive_->GetSettings().width;
        } else if (key == "PrimitiveWidthRandomness" && primitive_) {
            file >> primitive_->GetSettings().widthRandomness;
        } else if (key == "PrimitiveLength" && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();
            file >> settings.minLength >> settings.maxLength;
        } else if (key == "PrimitiveLifeTime" && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();
            file >> settings.minLifeTime >> settings.maxLifeTime;
        } else if (key == "PrimitiveMoveSpeed" && primitive_) {
            file >> primitive_->GetSettings().moveSpeed;
        } else if (key == "PrimitiveMoveSpeedRandomness" && primitive_) {
            file >> primitive_->GetSettings().moveSpeedRandomness;
        } else if (key == "PrimitiveRotationSpeed" && primitive_) {
            file >> primitive_->GetSettings().rotationSpeed;
        } else if (key == "PrimitiveRotationSpeedRandomness" && primitive_) {
            file >> primitive_->GetSettings().rotationSpeedRandomness;
        } else if (key == "PrimitiveDirection" && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();
            file >> settings.directionAngle >> settings.directionSpread;
        } else if (key == "PrimitiveSpawnRadius" && primitive_) {
            file >> primitive_->GetSettings().spawnRadius;
        } else if (key == "PrimitiveAcceleration" && primitive_) {
            readVector3(primitive_->GetSettings().acceleration);
        } else if (key == "PrimitiveEndScale" && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();
            file >> settings.endWidthScale >> settings.endLengthScale;
        } else if (key == "PrimitiveScaleEasePower" && primitive_) {
            file >> primitive_->GetSettings().scaleEasePower;
        } else if (key == "PrimitiveFadeInRatio" && primitive_) {
            file >> primitive_->GetSettings().fadeInRatio;
        } else if (key == "PrimitiveFadePower" && primitive_) {
            file >> primitive_->GetSettings().fadePower;
        } else if (key == "RingColor" && ring_) {
            readVector4(ring_->GetSettings().color);
        } else if (key == "RingEndColor" && ring_) {
            readVector4(ring_->GetSettings().endColor);
        } else if (key == "RingIntensity" && ring_) {
            file >> ring_->GetSettings().intensity;
        } else if (key == "RingScale" && ring_) {
            Ring::Settings& settings =
                ring_->GetSettings();
            file >> settings.startScale >> settings.endScale;
        } else if (key == "RingThickness" && ring_) {
            file >> ring_->GetSettings().thickness;
        } else if (key == "RingEndThickness" && ring_) {
            file >> ring_->GetSettings().endThickness;
        } else if (key == "RingLifeTime" && ring_) {
            file >> ring_->GetSettings().lifeTime;
        } else if (key == "RingFadeInRatio" && ring_) {
            file >> ring_->GetSettings().fadeInRatio;
        } else if (key == "RingEasePower" && ring_) {
            file >> ring_->GetSettings().easePower;
        } else if (key == "RingRotationSpeed" && ring_) {
            file >> ring_->GetSettings().rotationSpeed;
        } else if (key == "RingEdgeSoftness" && ring_) {
            file >> ring_->GetSettings().edgeSoftness;
        } else if (key == "RingGlowStrength" && ring_) {
            file >> ring_->GetSettings().glowStrength;
        } else if (key == "RingDistortion" && ring_) {
            Ring::Settings& settings =
                ring_->GetSettings();
            file
                >> settings.distortionStrength
                >> settings.distortionFrequency
                >> settings.distortionSpeed;
        } else if (key == "CylinderColor" && cylinder_) {
            readVector4(cylinder_->GetSettings().color);
        } else if (key == "CylinderEndColor" && cylinder_) {
            readVector4(cylinder_->GetSettings().endColor);
        } else if (key == "CylinderIntensity" && cylinder_) {
            file >> cylinder_->GetSettings().intensity;
        } else if (key == "CylinderRadius" && cylinder_) {
            file >> cylinder_->GetSettings().radius;
        } else if (key == "CylinderEndRadius" && cylinder_) {
            file >> cylinder_->GetSettings().endRadius;
        } else if (key == "CylinderHeight" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file >> settings.startHeight >> settings.endHeight;
        } else if (key == "CylinderRadiusScale" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file >> settings.bottomRadiusScale >> settings.topRadiusScale;
        } else if (key == "CylinderLifeTime" && cylinder_) {
            file >> cylinder_->GetSettings().lifeTime;
        } else if (key == "CylinderRiseDistance" && cylinder_) {
            file >> cylinder_->GetSettings().riseDistance;
        } else if (key == "CylinderEasePower" && cylinder_) {
            file >> cylinder_->GetSettings().easePower;
        } else if (key == "CylinderFadeInRatio" && cylinder_) {
            file >> cylinder_->GetSettings().fadeInRatio;
        } else if (key == "CylinderFadePower" && cylinder_) {
            file >> cylinder_->GetSettings().fadePower;
        } else if (key == "CylinderTwist" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file >> settings.twistAmount >> settings.twistSpeed;
        } else if (key == "CylinderNoise" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file
                >> settings.noiseStrength
                >> settings.noiseFrequency
                >> settings.noiseSpeed;
        } else if (key == "CylinderVerticalFade" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file >> settings.topFade >> settings.bottomFade;
        } else if (key == "CylinderPositionOffset" && cylinder_) {
            readVector3(cylinder_->GetSettings().positionOffset);
        } else if (key == "SakuraColor" && particleManager_) {
            readVector4(particleManager_->GetSakuraSettings().color);
        } else if (key == "SakuraSubColor" && particleManager_) {
            readVector4(particleManager_->GetSakuraSettings().subColor);
        } else if (key == "SakuraPetalCount" && particleManager_) {
            file >> particleManager_->GetSakuraSettings().petalCount;
        } else if (key == "SakuraSize" && particleManager_) {
            auto& settings =
                particleManager_->GetSakuraSettings();
            file >> settings.minSize >> settings.maxSize;
        } else if (key == "SakuraSpawnRadius" && particleManager_) {
            file >> particleManager_->GetSakuraSettings().spawnRadius;
        } else if (key == "SakuraSpeed" && particleManager_) {
            auto& settings =
                particleManager_->GetSakuraSettings();
            file >> settings.spreadSpeed >> settings.upwardSpeed;
        } else if (key == "SakuraLifeTime" && particleManager_) {
            auto& settings =
                particleManager_->GetSakuraSettings();
            file >> settings.minLifeTime >> settings.maxLifeTime;
        } else if (key == "SakuraGravity" && particleManager_) {
            file >> particleManager_->GetSakuraSettings().gravity;
        } else if (key == "SakuraRotationSpeed" && particleManager_) {
            file >> particleManager_->GetSakuraSettings().rotationSpeed;
        } else if (key == "SakuraFlash" && particleManager_) {
            auto& settings =
                particleManager_->GetSakuraSettings();
            file >> settings.flashSize >> settings.flashLifeTime;
        } else {
            std::string unusedLine;
            std::getline(file, unusedLine);
        }
    }

    size_ =
        std::clamp(
            size_,
            0.1f,
            5.0f
        );

    if (primitive_) {
        Primitive::Settings& settings =
            primitive_->GetSettings();

        settings.count =
            std::clamp(
                settings.count,
                1,
                128
            );
    }

    if (ring_) {
        ring_->SetThickness(
            ring_->GetSettings().thickness
        );
    }

    UpdateActiveFlags();

    message_ =
        "Loaded V" +
        std::to_string(presetVersion) +
        " : " +
        safeName;

    return true;
}

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