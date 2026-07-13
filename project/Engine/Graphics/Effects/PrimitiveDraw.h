void Primitive::Draw()
{
    if (!isActive_) {
        return;
    }

    if (particles_.empty()) {
        return;
    }

    auto commandList =
        dxCommon_->GetCommandList();

    assert(
        pipelineState_ != nullptr &&
        "Primitive PipelineState not created!"
    );

    commandList->SetPipelineState(
        pipelineState_.Get()
    );

    commandList->SetGraphicsRootSignature(
        rootSignature_.Get()
    );

    commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    commandList->IASetVertexBuffers(
        0,
        1,
        &vertexBufferView_
    );

    commandList->IASetVertexBuffers(
        1,
        1,
        &instancingBufferView_
    );

    auto textureHandle =
        textureManager_->GetSrvHandleGPU(
            textureHandle_
        );

    commandList->SetGraphicsRootDescriptorTable(
        0,
        textureHandle
    );

    uint32_t particleCount =
        static_cast<uint32_t>(
            particles_.size()
            );

    if (particleCount > kMaxParticles) {
        particleCount = kMaxParticles;
    }

    commandList->DrawInstanced(
        6,
        particleCount,
        0,
        0
    );
}


