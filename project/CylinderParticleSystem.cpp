#include "CylinderParticleSystem.h"

void CylinderParticleSystem::Emit(const Vector3& position) {
    if (particles_.size() >= kMaxParticles) {
        return;
    }

    Particle particle{};

    particle.transform.translate = position;
    particle.transform.scale = { 0.8f, 0.1f, 0.8f };
    particle.transform.rotate = { 0.0f, 0.0f, 0.0f };

    particle.color = { 0.4f, 0.7f, 1.0f, 0.6f };

    particle.lifeTime = 0.0f;
    particle.maxTime = 0.7f;

    particles_.push_back(particle);
}

void CylinderParticleSystem::Update() {
    for (auto it = particles_.begin(); it != particles_.end();) {
        it->lifeTime += 1.0f / 60.0f;

        if (it->lifeTime >= it->maxTime) {
            it = particles_.erase(it);
            continue;
        }

        float t = it->lifeTime / it->maxTime;

        // 時間経過で上方向に伸ばす。
        float height = 0.1f + t * maxHeight_;

        it->transform.scale = {
            0.8f,
            height,
            0.8f
        };

        // 寿命が近づくほど透明にする。
        it->color.w = 0.6f * (1.0f - t);

        ++it;
    }
}