#pragma once

#include "Engine.h"
#include "SceneManager.h"

// MyGame はゲーム全体の進行役。
// Engine の初期化と SceneManager の更新・描画をつなぐ。
class MyGame {
public:
    void Initialize();
    void Update();
    void Draw();
    void Finalize();

    bool IsRunning();

private:
    Engine engine_;
    SceneManager sceneManager_;
};