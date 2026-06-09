#pragma once

#include "GameSystem.h"
#include "GameScene.h"

class MyGame {
public:
    void Initialize();
    void Update();
    void Draw();
    void Finalize();

    bool IsRunning();

private:
    GameSystem gameSystem_;
    GameScene gameScene_;
};