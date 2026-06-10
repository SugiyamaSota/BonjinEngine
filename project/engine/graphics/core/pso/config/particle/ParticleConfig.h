// ModelPipelineConfig.h
#pragma once
#include "../IPipelineConfig.h"
#include "rootSignatureBuilder/RootSignatureBuilder.h"

class ParticleConfigEx : public IPipelineConfig {
public:

    ParticleConfigEx() {
        //__debugbreak();
    }

    const wchar_t* GetShaderPath(ShaderStage stage) const override;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* device)override;

    std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputElements() override;

    void CustomSetupPSO(
        GraphicsPipelineStateBuilder& psoBuilder,
        D3D12_FILL_MODE fillMode,
        D3D12_CULL_MODE cullMode) override;
};