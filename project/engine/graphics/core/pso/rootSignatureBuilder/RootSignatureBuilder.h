#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

class RootSignatureBuilder {
public:
    RootSignatureBuilder();

    // ルートパラメータを追加するメソッド
    RootSignatureBuilder& AddRootParameter(const D3D12_ROOT_PARAMETER& param);
    // ディスクリプタレンジを追加するメソッド (ディスクリプタテーブル用)
    RootSignatureBuilder& AddDescriptorRange(const D3D12_DESCRIPTOR_RANGE& range);
    // 静的サンプラーを追加するメソッド
    RootSignatureBuilder& AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler);
    // フラグを設定するメソッド
    RootSignatureBuilder& SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);

    // ルートシグネチャを構築して返す
    Microsoft::WRL::ComPtr<ID3D12RootSignature> Build(ID3D12Device* device);

private:
    std::vector<D3D12_ROOT_PARAMETER> rootParameters_;
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges_; // ディスクリプタテーブルで使用されるレンジ
    D3D12_ROOT_SIGNATURE_FLAGS flags_;
};