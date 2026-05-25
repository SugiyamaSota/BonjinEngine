#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include<dxcapi.h>

enum class BlendMode {
    kNone,        // ブレンドなし (不透明)
    kNormal,      // 通常αブレンド。Src * SrcA + Dest * (1 - SrcA)
    kAdd,         // 加算。Src * SrcA + Dest * 1
    kSubtract,    // 減算。Dest * 1 - Src * SrcA
    kMutiliy,     // 乗算。Src * 0 + Dest * Src
    kScreen,      // スクリーン。Src * (1 - Dest) + Dest * 1

    // ⭐ 要素の総数
    kCount,
};

class GraphicsPipelineStateBuilder {
public:
    GraphicsPipelineStateBuilder();

    GraphicsPipelineStateBuilder& SetRootSignature(ID3D12RootSignature* rootSignature);
    GraphicsPipelineStateBuilder& SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& inputLayout);
    GraphicsPipelineStateBuilder& SetVertexShader(IDxcBlob* vsBlob);
    GraphicsPipelineStateBuilder& SetPixelShader(IDxcBlob* psBlob);
    GraphicsPipelineStateBuilder& SetBlendState(const D3D12_BLEND_DESC& blendState);
    GraphicsPipelineStateBuilder& SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerState);
    GraphicsPipelineStateBuilder& SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilState);
    GraphicsPipelineStateBuilder& AddRenderTargetFormat(DXGI_FORMAT format);
    GraphicsPipelineStateBuilder& SetDepthStencilViewFormat(DXGI_FORMAT format);
    GraphicsPipelineStateBuilder& SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
    GraphicsPipelineStateBuilder& SetSampleDesc(const DXGI_SAMPLE_DESC& sampleDesc);
    GraphicsPipelineStateBuilder& SetSampleMask(UINT mask);

    Microsoft::WRL::ComPtr<ID3D12PipelineState> Build(ID3D12Device* device);

    GraphicsPipelineStateBuilder& SetBlendMode(BlendMode blendMode);

private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_;
    std::vector<DXGI_FORMAT> rtvFormats_;
};