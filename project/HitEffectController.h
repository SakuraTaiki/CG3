#pragma once
#include <array>
#include <string>
#include <vector>

#include "MyMath.h"
#include "Primitive.h"
#include "Ring.h"
#include "Cylinder.h"
#include "ParticleManager.h"

class HitEffectController
{
public:
    enum class Type {
        Fire,
        Sakura
    };

    void Initialize(
        Primitive* primitive,
        Ring* ring,
        Cylinder* cylinder,
        ParticleManager* particleManager
    );

    void Emit(const Vector3& position);
    void ApplyFirePreset();

    bool SavePreset(const std::string& presetName);
    bool LoadPreset(const std::string& presetName);
    void RefreshPresetList();

    void UpdateActiveFlags();

    Type& GetType() { return type_; }
    float& GetSize() { return size_; }
    Vector3& GetPosition() { return position_; }

    bool& EnablePrimitive() { return enablePrimitive_; }
    bool& EnableRing() { return enableRing_; }
    bool& EnableCylinder() { return enableCylinder_; }

    std::vector<std::string>& GetPresetNames() { return presetNames_; }
    int& GetSelectedPreset() { return selectedPreset_; }
    std::array<char, 64>& GetPresetNameBuffer() { return presetNameBuffer_; }
    const std::string& GetMessage() const { return message_; }

private:
    std::string MakeSafePresetName(const std::string& name) const;

private:
    Primitive* primitive_ = nullptr;
    Ring* ring_ = nullptr;
    Cylinder* cylinder_ = nullptr;
    ParticleManager* particleManager_ = nullptr;

    Type type_ = Type::Fire;
    float size_ = 1.0f;

    Vector3 position_ = {
        0.0f,
        3.0f,
        0.0f
    };

    bool enablePrimitive_ = true;
    bool enableRing_ = true;
    bool enableCylinder_ = true;

    std::vector<std::string> presetNames_;
    int selectedPreset_ = 0;

    std::array<char, 64> presetNameBuffer_ = {
        'F', 'i', 'r', 'e', '\0'
    };

    std::string message_;
};

