#include "CylinderParticleSystem.h"

#include <cmath>

void CylinderParticleSystem::Emit(
    const Vector3& position
) {
    if (particles_.size() >= kMaxParticles) {
        return;
    }

    Particle particle{};

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
        0.0f,
        0.0f,
        0.0f
    };

    particle.color =
        settings_.color;

    particle.lifeTime = 0.0f;

    particle.maxTime =
        settings_.lifeTime;

    particles_.push_back(particle);
}

void CylinderParticleSystem::Update()
{
    const float deltaTime =
        1.0f / 60.0f;

    for (
        auto iterator = particles_.begin();
        iterator != particles_.end();
        ) {
        iterator->lifeTime += deltaTime;

        if (iterator->lifeTime >=
            iterator->maxTime) {

            iterator =
                particles_.erase(iterator);

            continue;
        }

        const float time =
            iterator->lifeTime /
            iterator->maxTime;

        // 最初に速く伸ばすEaseOut
        const float easedTime =
            1.0f -
            std::pow(
                1.0f - time,
                settings_.easePower
            );

        const float height =
            settings_.startHeight +
            (
                settings_.endHeight -
                settings_.startHeight
                ) * easedTime;

        iterator->transform.scale = {
            settings_.radius,
            height,
            settings_.radius
        };

        iterator->transform.translate =
            iterator->startPosition;

        iterator->transform.translate.y +=
            settings_.riseDistance *
            easedTime;

        iterator->color.x =
            settings_.color.x *
            settings_.intensity;

        iterator->color.y =
            settings_.color.y *
            settings_.intensity;

        iterator->color.z =
            settings_.color.z *
            settings_.intensity;

        iterator->color.w =
            settings_.color.w *
            std::pow(
                1.0f - time,
                settings_.fadePower
            );

        ++iterator;
    }
}