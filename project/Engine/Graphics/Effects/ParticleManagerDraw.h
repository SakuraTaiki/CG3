void ParticleManager::Draw() {
    if (particles_.empty()) return;

    auto commandList = dxCommon_->GetCommandList();

    
    assert(pipelineState_ != nullptr && "PipelineState not created!");

    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetVertexBuffers(1, 1, &instancingBufferView_);

    auto srvHandle = textureManager_->GetSrvHandleGPU(textureHandle_);
    commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

    uint32_t count = (uint32_t)particles_.size();
    if (count > kMaxParticles) count = kMaxParticles;
    commandList->DrawInstanced(6, count, 0, 0);
}

