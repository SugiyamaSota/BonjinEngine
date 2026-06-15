#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include "graphicsPipelineStateBuilder/GraphicsPipelineStateBuilder.h"

enum class ShaderStage {
    kVertex,
    kPixel,
    kCount,
};

class IPipelineConfig {
public:
    virtual ~IPipelineConfig() = default;

    virtual const wchar_t* GetShaderPath(ShaderStage stage) const = 0;

    // 固有のルートシグネチャを作成して返す
    virtual Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* device) = 0;

    // 固有のインプットレイアウト要素を返す
    virtual std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputElements() = 0;

    // プリミティブ固有のPSOカスタマイズ（トポロジー、ラスタライザー、デプス設定など）を行う
    virtual void CustomSetupPSO(
        GraphicsPipelineStateBuilder& psoBuilder,
        D3D12_FILL_MODE fillMode,
        D3D12_CULL_MODE cullMode) = 0;
};