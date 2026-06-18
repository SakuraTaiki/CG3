#pragma once

#include <cstdint>
#include <list>

#include "MyMath.h"

// Cylinder 用の粒子管理クラス。
// 発生、寿命、拡大、フェードアウトなどの動きだけを担当する。
class CylinderParticleSystem {
public:
    struct Particle {
        Transform transform;
        Vector4 color;
        float lifeTime;
        float maxTime;
    };

    void Emit(const Vector3& position);
    void Update();

    const std::list<Particle>& GetParticles() const { return particles_; }
    bool IsEmpty() const { return particles_.empty(); }

private:
    static const uint32_t kMaxParticles = 64;

    std::list<Particle> particles_;

    // 時間経過で伸びる最大高さ。
    float maxHeight_ = 2.5f;
};