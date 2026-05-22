#include "RootSignatureBuilder.h"
#include <cassert>
#include <vector>

RootSignatureBuilder::RootSignatureBuilder() : flags_(D3D12_ROOT_SIGNATURE_FLAG_NONE) {}

RootSignatureBuilder& RootSignatureBuilder::AddRootParameter(const D3D12_ROOT_PARAMETER& param) {
    rootParameters_.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddDescriptorRange(const D3D12_DESCRIPTOR_RANGE& range) {
    descriptorRanges_.push_back(range);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler) {
    staticSamplers_.push_back(sampler);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags) {
    flags_ = flags;
    return *this;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(ID3D12Device* device) {
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.pParameters = rootParameters_.data();
    descriptionRootSignature.NumParameters = static_cast<UINT>(rootParameters_.size());
    descriptionRootSignature.pStaticSamplers = staticSamplers_.data();
    descriptionRootSignature.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());
    descriptionRootSignature.Flags = flags_;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(
        &descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1,
        signatureBlob.GetAddressOf(),
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        }
        assert(false && "Failed to serialize root signature!");
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    hr = device->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(rootSignature.GetAddressOf())
    );
    assert(SUCCEEDED(hr));

    return rootSignature;
}