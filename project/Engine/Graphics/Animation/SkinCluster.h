#pragma once

#include "Model.h"
#include "Skelton.h"
#include "SrvManager.h"
#include <array>
#include <vector>
#include <wrl.h>
#include <d3d12.h>

const uint32_t kNumMaxInfluence = 4;

struct VertexInfluence {
    std::array<float, kNumMaxInfluence> weights;
    std::array<int32_t, kNumMaxInfluence> jointIndices;
};

struct WellForGPU {
    Matrix4x4 skeletonSpaceMatrix;
    Matrix4x4 skeletonSpaceInverseTransposeMatrix;
};

struct SkinCluster {
    std::vector<Matrix4x4> inverseBindPoseMatrices;

    Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
    D3D12_VERTEX_BUFFER_VIEW influenceBufferView{};
    VertexInfluence* mappedInfluence = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
    WellForGPU* mappedPalette = nullptr;

    uint32_t paletteSrvIndex = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE paletteSrvHandle{};
};

bool CreateSkinCluster(
    SkinCluster& skinCluster,
    ID3D12Device* device,
    SrvManager* srvManager,
    const Skeleton& skeleton,
    const Model& model
);

void UpdateSkinCluster(
    SkinCluster& skinCluster,
    const Skeleton& skeleton
);