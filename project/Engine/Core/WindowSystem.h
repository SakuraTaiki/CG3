#pragma once

#include <memory>
#include "WinApp.h"

// ウィンドウ生成とメッセージ処理を担当するクラス。
// WinApp の所有者。
class WindowSystem {
public:
    void Initialize();
    void Finalize();

    // true が返ったら終了メッセージあり。
    bool ProcessMessage();

    WinApp* GetWinApp() const { return winApp_.get(); }

private:
    std::unique_ptr<WinApp> winApp_;
};