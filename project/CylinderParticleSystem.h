#pragma once

#include <cstdint>
#include <list>

#include "MyMath.h"

// Cylinder 用の粒子管理クラス。
// 発生、寿命、拡大、フェードアウトなどの動きだけを担当する。
class CylinderParticleSystem {
public:
    
    struct Particle
    {
        Transform transform;

        // 上昇処理用
        Vector3 startPosition;

        Vector4 color;

        float lifeTime;
        float maxTime;
    };


    struct Settings
    {
        Vector4 color = {
            0.4f,
            0.7f,
            1.0f,
            0.6f
        };

        float intensity = 1.0f;

        // 円柱の半径
        float radius = 0.8f;

        // 発生直後の高さ
        float startHeight = 0.1f;

        // 最終的な高さ
        float endHeight = 2.5f;

        // 表示時間
        float lifeTime = 0.7f;

        // 円柱全体が上へ移動する距離
        float riseDistance = 0.0f;

        // 高さの伸び方
        float easePower = 1.0f;

        // 透明になる速さ
        float fadePower = 1.0f;

        // HitEffect中心からの相対位置
        Vector3 positionOffset = {
            0.0f,
            0.0f,
            0.0f
        };
    };

    Settings& GetSettings()
    {
        return settings_;
    }

    const Settings& GetSettings() const
    {
        return settings_;
    }

   

    void Emit(const Vector3& position);
    void Update();

    const std::list<Particle>& GetParticles() const { return particles_; }
    bool IsEmpty() const { return particles_.empty(); }

private:
    static const uint32_t kMaxParticles = 64;

    std::list<Particle> particles_;

   

    Settings settings_{};
};