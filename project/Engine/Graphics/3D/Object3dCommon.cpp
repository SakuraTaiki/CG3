#include "Object3dCommon.h"
#include "Logger.h" // ログ用
#include <cassert>
#include <cmath>

using namespace Microsoft::WRL;

void Object3dCommon::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;

    // ログを出して進行状況を確認
    OutputDebugStringA("Object3dCommon::Initialize Start\n");

    CreateRootSignature();
    // ルートシグネチャができているかチェック
    if (rootSignature_) {
        OutputDebugStringA("CreateRootSignature: OK\n");
    } else {
        OutputDebugStringA("CreateRootSignature: FAILED (RootSignature is NULL)\n");
        assert(false && "RootSignature creation failed!");
    }

    CreateGraphicsPipeline();
    // パイプラインステートができているかチェック
    if (pipelineState_) {
        OutputDebugStringA("CreateGraphicsPipeline: OK\n");
    } else {
        OutputDebugStringA("CreateGraphicsPipeline: FAILED (PipelineState is NULL)\n");
        // ここで落ちる可能性が高い
    }

    CreateLightBuffer();
    CreateSpotLightBuffer();
    CreatePointLightBuffer();
    SetDefaultLight();

    
    OutputDebugStringA("Object3dCommon::Initialize Finish\n");
}

void Object3dCommon::PreDraw() {
    auto commandList = dxCommon_->GetCommandList();

    // ここで落ちる場合、Initializeで作成失敗している
    if (!rootSignature_ || !pipelineState_) {
        assert(false && "RootSignature or PipelineState is NULL in PreDraw!");
        return;
    }

    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(pipelineState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Object3dCommon::SetDefaultLight() {
    if (lightData_) {
        lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightData_->direction = { 0.0f, -1.0f, 0.0f };
        lightData_->intensity = 1.0f;
    }

    if (spotLightData_) {
        spotLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        spotLightData_->position = { 0.0f, 5.0f, 0.0f };
        spotLightData_->intensity = 4.0f;

        spotLightData_->direction =
            Math::Normalize(Vector3{ 0.0f, -1.0f, 0.0f });

        spotLightData_->distance = 15.0f;
        spotLightData_->decay = 2.0f;

        // 外側30度
        spotLightData_->cosAngle =
            std::cos(30.0f * 3.1415926535f / 180.0f);

        // 20度から減衰開始
        spotLightData_->cosFalloffStart =
            std::cos(20.0f * 3.1415926535f / 180.0f);

        spotLightData_->padding = 0.0f;
    }


    if (pointLightData_) {
        pointLightData_->color =
        { 1.0f, 1.0f, 1.0f, 1.0f };

        pointLightData_->position =
        { 0.0f, 3.0f, 0.0f };

        pointLightData_->intensity = 2.0f;
        pointLightData_->radius = 10.0f;
        pointLightData_->decay = 2.0f;

        pointLightData_->padding[0] = 0.0f;
        pointLightData_->padding[1] = 0.0f;
    }

}

void Object3dCommon::CreateRootSignature() {
    OutputDebugStringA("CreateRootSignature Start\n");

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // 0: Material       PS b0
// 1: Transform      VS b0
// 2: Light          PS b1
// 3: Camera         PS b2
// 4: Texture2D      PS t0
// 5: Environment    PS t1
    D3D12_ROOT_PARAMETER rootParameters[9] = {};

    // 0. Material
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 1. Transform
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // 2. Light
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[2].Descriptor.ShaderRegister = 1;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 3. Camera
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].Descriptor.ShaderRegister = 2;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 4. Texture2D PS t0
    D3D12_DESCRIPTOR_RANGE textureRange{};
    textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    textureRange.BaseShaderRegister = 0;
    textureRange.NumDescriptors = 1;
    textureRange.RegisterSpace = 0;
    textureRange.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[4].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[4].DescriptorTable.pDescriptorRanges = &textureRange;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 5. Environment TextureCube PS t1
    D3D12_DESCRIPTOR_RANGE environmentRange{};
    environmentRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    environmentRange.BaseShaderRegister = 1;
    environmentRange.NumDescriptors = 1;
    environmentRange.RegisterSpace = 0;
    environmentRange.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[5].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[5].DescriptorTable.pDescriptorRanges = &environmentRange;
    rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 6. MatrixPalette VS t0
    D3D12_DESCRIPTOR_RANGE matrixPaletteRange{};
    matrixPaletteRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    matrixPaletteRange.BaseShaderRegister = 0;
    matrixPaletteRange.NumDescriptors = 1;
    matrixPaletteRange.RegisterSpace = 0;
    matrixPaletteRange.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[6].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[6].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[6].DescriptorTable.pDescriptorRanges = &matrixPaletteRange;
    rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // 7. SpotLight PS b3
    rootParameters[7].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_CBV;

    rootParameters[7].Descriptor.ShaderRegister = 3;
    rootParameters[7].Descriptor.RegisterSpace = 0;
    rootParameters[7].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    // 8. PointLight PS b4
    rootParameters[8].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_CBV;

    rootParameters[8].Descriptor.ShaderRegister = 4;
    rootParameters[8].Descriptor.RegisterSpace = 0;

    rootParameters[8].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;


    D3D12_STATIC_SAMPLER_DESC staticSampler{};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumStaticSamplers = 1;
    descriptionRootSignature.pStaticSamplers = &staticSampler;

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(
        &descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        assert(false);
    }

    hr = dxCommon_->GetDevice()->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));

    assert(SUCCEEDED(hr));
}

void Object3dCommon::CreateGraphicsPipeline() {
    OutputDebugStringA("CreateGraphicsPipeline Start\n");

    // Vertex Shader
    auto vsBlob = dxCommon_->CompileShader(L"Resources/shaders/hlsl/Object3d.VS.hlsl", L"vs_6_0");
    assert(vsBlob); // ここは通過しているはず

    // Pixel Shader
    auto psBlob = dxCommon_->CompileShader(L"Resources/shaders/hlsl/Object3d.PS.hlsl", L"ps_6_0");
    assert(psBlob); // ここも通過しているはず

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    { "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "INDEX",    0, DXGI_FORMAT_R32G32B32A32_SINT,  1, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature_.Get(); // ★ここがNULLだとPSO作成に失敗する
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    auto& target = psoDesc.BlendState.RenderTarget[0];
    target.BlendEnable = FALSE;
    target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    OutputDebugStringA("Calling CreateGraphicsPipelineState...\n");

    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));

    if (FAILED(hr)) {
        OutputDebugStringA("CreateGraphicsPipelineState Failed!! HRESULT error.\n");
        // もしここで落ちるなら、ルートシグネチャかシェーダーblobがおかしい
        assert(false);
    } else {
        OutputDebugStringA("CreateGraphicsPipelineState Success!\n");
    }
}

void Object3dCommon::CreateLightBuffer() {
    auto device = dxCommon_->GetDevice();
    D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = (sizeof(DirectionalLight) + 0xff) & ~0xff;
    resDesc.Height = 1; resDesc.DepthOrArraySize = 1; resDesc.MipLevels = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; resDesc.SampleDesc.Count = 1;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&lightResource_));
    lightResource_->Map(0, nullptr, (void**)&lightData_);
}

void Object3dCommon::CreateSpotLightBuffer()
{

    auto device = dxCommon_->GetDevice();

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty =
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference =
        D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width =
        (sizeof(SpotLight) + 0xff) & ~0xff;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&spotLightResource_)
    );

    assert(SUCCEEDED(hr));

    hr = spotLightResource_->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&spotLightData_)
    );

    assert(SUCCEEDED(hr));

}

void Object3dCommon::CreatePointLightBuffer()
{


    auto device = dxCommon_->GetDevice();

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension =
        D3D12_RESOURCE_DIMENSION_BUFFER;

    resourceDesc.Width =
        (sizeof(PointLight) + 0xff) & ~0xff;

    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout =
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr =
        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pointLightResource_)
        );

    assert(SUCCEEDED(hr));

    hr = pointLightResource_->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&pointLightData_)
    );

    assert(SUCCEEDED(hr));


}
