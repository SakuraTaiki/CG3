#pragma once

#include <memory>
#include "Input.h"

class WinApp;
class EngineContext;

// 入力機能を担当するクラス。
// Input の所有と毎フレーム更新を行う。
class InputSystem {
public:
    void Initialize(WinApp* winApp, EngineContext* context);
    void Finalize();
    void Update();

    Input* GetInput() const { return input_.get(); }

private:
    std::unique_ptr<Input> input_;
};