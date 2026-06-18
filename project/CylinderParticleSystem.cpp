#include "CylinderParticleSystem.h"

#include <algorithm>
#include <cmath>

void CylinderParticleSystem::Emit(
    const Vector3& position
) {
    if (particles_.size() >= kMaxParticles) {
        return;
    }

    Particle particle{};

    // 発生時の設定を保存
    particle.settings =
        settings_;

    particle.startPosition = {
        position.x +
            settings_.positionOffset.x,

        position.y +
            settings_.positionOffset.y,

        position.z +
            settings_.positionOffset.z
    };

    particle.transform.translate =
        particle.startPosition;

    particle.transform.scale = {
        settings_.radius,
        settings_.startHeight,
        settings_.radius
    };

    particle.transform.rotate = {
        0.0f, 0.0f, 0.0f
    };

    particle.color =
        settings_.color;

    particle.lifeTime = 0.0f;

    particle.maxTime =
        std::max(
            settings_.lifeTime,
            0.001f
        );

    particle.progress = 0.0f;
    particle.twistPhase = 0.0f;
    particle.noisePhase = 0.0f;

    particles_.push_back(particle);
}

void CylinderParticleSystem::Update()
{
    constexpr float deltaTime =
        1.0f / 60.0f;

    for (
        auto iterator = particles_.begin();
        iterator != particles_.end();
        ) {
        Particle& particle =
            *iterator;

        particle.lifeTime +=
            deltaTime;

        if (
            particle.lifeTime >=
            particle.maxTime
            ) {
            iterator =
                particles_.erase(iterator);

            continue;
        }

        const Settings& settings =
            particle.settings;

        const float time =
            std::clamp(
                particle.lifeTime /
                particle.maxTime,
                0.0f,
                1.0f
            );

        particle.progress =
            time;

        const float easedTime =
            1.0f -
            std::pow(
                1.0f - time,
                std::max(
                    settings.easePower,
                    0.01f
                )
            );

        const float radius =
            settings.radius +
            (
                settings.endRadius -
                settings.radius
                ) *
            easedTime;

        const float height =
            settings.startHeight +
            (
                settings.endHeight -
                settings.startHeight
                ) *
            easedTime;

        particle.transform.scale = {
            radius,
            height,
            radius
        };

        particle.transform.translate =
            particle.startPosition;

        particle.transform.translate.y +=
            settings.riseDistance *
            easedTime;

        particle.twistPhase +=
            settings.twistSpeed *
            deltaTime;

        particle.noisePhase +=
            settings.noiseSpeed *
            deltaTime;

        float fadeIn = 1.0f;

        if (settings.fadeInRatio > 0.0f) {
            fadeIn =
                std::clamp(
                    time /
                    settings.fadeInRatio,
                    0.0f,
                    1.0f
                );
        }

        const float fadeOut =
            std::pow(
                1.0f - time,
                std::max(
                    settings.fadePower,
                    0.01f
                )
            );

        const float red =
            settings.color.x +
            (
                settings.endColor.x -
                settings.color.x
                ) *
            time;

        const float green =
            settings.color.y +
            (
                settings.endColor.y -
                settings.color.y
                ) *
            time;

        const float blue =
            settings.color.z +
            (
                settings.endColor.z -
                settings.color.z
                ) *
            time;

        const float alpha =
            settings.color.w +
            (
                settings.endColor.w -
                settings.color.w
                ) *
            time;

        particle.color = {
            red * settings.intensity,
            green * settings.intensity,
            blue * settings.intensity,
            alpha * fadeIn * fadeOut
        };

        ++iterator;
    }
}