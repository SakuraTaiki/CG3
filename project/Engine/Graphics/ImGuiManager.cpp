#include "ImGuiManager.h"

#ifdef USE_IMGUI

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"

#endif

void ImGuiManager::Initialize(
    [[maybe_unused]] DirectXCommon* dxCommon,
    [[maybe_unused]] SrvManager* srvManager,
    [[maybe_unused]] WinApp* winApp)
{
#ifdef USE_IMGUI

    dxCommon_ = dxCommon;
    srvManager_ = srvManager;

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(winApp->GetHwnd());

    uint32_t srvIndex = srvManager_->Allocate();

    ImGui_ImplDX12_Init(
        dxCommon_->GetDevice(),
        static_cast<int>(dxCommon_->GetSwapChainResourcesNum()),
        dxCommon_->GetRTVFormat(),
        srvManager_->GetDescriptorHeap(),
        srvManager_->GetCPUDescriptorHandle(srvIndex),
        srvManager_->GetGPUDescriptorHandle(srvIndex)
    );

#endif
}

void ImGuiManager::Finalize()
{
#ifdef USE_IMGUI

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

#endif
}

void ImGuiManager::Begin()
{
#ifdef USE_IMGUI

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

#endif
}

void ImGuiManager::End()
{
#ifdef USE_IMGUI

    ImGui::Render();

#endif
}

void ImGuiManager::Draw()
{
#ifdef USE_IMGUI

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    ID3D12DescriptorHeap* ppHeaps[] = {
        srvManager_->GetDescriptorHeap()
    };

    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    ImGui_ImplDX12_RenderDrawData(
        ImGui::GetDrawData(),
        commandList);

#endif
}