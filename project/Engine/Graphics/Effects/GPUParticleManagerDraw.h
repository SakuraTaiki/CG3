void GPUParticleManager::Draw()
{

    if (!settings_.enabled)
    {
        return;
    }

    ID3D12GraphicsCommandList* commandList =
        dxCommon_->GetCommandList();

    TransitionParticleResource(
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    commandList->SetPipelineState(
        graphicsPipelineState_.Get()
    );
    commandList->SetGraphicsRootSignature(
        graphicsRootSignature_.Get()
    );
    commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );
    commandList->IASetVertexBuffers(
        0,
        1,
        &vertexBufferView_
    );

    commandList->SetGraphicsRootDescriptorTable(
        0,
        textureManager_->GetSrvHandleGPU(textureHandle_)
    );
    commandList->SetGraphicsRootDescriptorTable(
        1,
        srvManager_->GetGPUDescriptorHandle(particleSrvIndex_)
    );
    commandList->SetGraphicsRootConstantBufferView(
        2,
        viewProjectionBuffer_->GetGPUVirtualAddress()
    );

    commandList->DrawInstanced(
        6,
        kMaxParticles,
        0,
        0
    );
}

