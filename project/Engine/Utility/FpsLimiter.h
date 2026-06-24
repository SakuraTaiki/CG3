#pragma once

#include <chrono>

// フレームレート固定を担当するクラス。
// DirectX の描画処理とは直接関係ないため分離する。
class FpsLimiter {
public:
    void Initialize();

    // 1フレームの最後に呼ぶ。
    // 60FPS になるように必要なら待機する。
    void Update();

private:
    std::chrono::steady_clock::time_point reference_;
};