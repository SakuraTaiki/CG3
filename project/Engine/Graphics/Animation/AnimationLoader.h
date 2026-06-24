#pragma once

#include <string>

#include "Animation.h"

// Animation ファイル読み込み専用クラス。
// Assimp への依存を Animation 本体から分離する。
class AnimationLoader {
public:
    static Animation Load(
        const std::string& directoryPath,
        const std::string& filename
    );
};