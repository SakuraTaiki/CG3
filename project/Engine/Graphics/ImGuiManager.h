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

private:
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
};

