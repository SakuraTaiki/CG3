#pragma once

#include "EngineContext.h"
#include "GraphicsSystem.h"
#include "InputSystem.h"
#include "WindowSystem.h"

// Engine は、アプリ全体で使う基盤システムをまとめるクラス。
// Window / Input / Graphics を所有し、Scene に渡す Context も持つ。
class Engine {
public:
    void Initialize();
    void Finalize();

    // 1フレームの最初に行う共通更新。
    // 現在は入力更新を担当する。
    void BeginFrame();

    // false になったらメインループを終了する。
    bool IsRunning();

    // Scene 初期化時に渡すための Context。
    EngineContext* GetContext() { return &context_; }
    const EngineContext* GetContext() const { return &context_; }

private:
    EngineContext context_;

    WindowSystem windowSystem_;
    InputSystem inputSystem_;
    GraphicsSystem graphicsSystem_;
};