#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "WinApp.h"

class ImGuiManager
{
public:
    void Initialize(
        DirectXCommon* dxCommon,
        SrvManager* srvManager,
        WinApp* winApp);

    void Finalize();

    void Begin();

    void End();

    void Draw();

    void UpdateGameViewTexture();

    uint32_t GetGameViewSrvIndex() const {
        return gameViewSrvIndex_;
    }

private:
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    uint32_t gameViewSrvIndex_ = 0;
};

