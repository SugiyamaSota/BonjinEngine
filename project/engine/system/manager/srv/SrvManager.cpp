#include "SrvManager.h"
#include<cassert>

SrvManager* SrvManager::GetInstance() {
    static SrvManager instance;
    return &instance;
}

void SrvManager::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    // 0番はImGuiで使用されている想定で、1番から割り当てを開始する
    useCount_ = 1;
}

uint32_t SrvManager::Allocate() {
    assert(useCount_ < kMaxSRVCount);
    uint32_t index = useCount_;
    useCount_++;
    return index;
}

void SrvManager::CreateTextureSrv(uint32_t index, ID3D12Resource* resource) {
    auto desc = resource->GetDesc(); // リソースの設定を取得

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format; // ここでリソースのフォーマットをセット
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (desc.DepthOrArraySize == 6) {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = UINT_MAX;
        srvDesc.TextureCube.ResourceMinLODClamp = 0.f;
    } else {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    }
   

    dxCommon_->GetDevice()->CreateShaderResourceView(
        resource, &srvDesc, GetCPUHandle(index)
    );
}

void SrvManager::CreateStructuredBufferSrv(uint32_t index, ID3D12Resource* resource, uint32_t numElements, uint32_t structureByteStride) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = structureByteStride;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    dxCommon_->GetDevice()->CreateShaderResourceView(
        resource, &srvDesc, GetCPUHandle(index)
    );
}

void SrvManager::PreDraw() {
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSRVDescriptorHeap() };
    dxCommon_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUHandle(uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = dxCommon_->GetSRVDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += (index * dxCommon_->GetSRVSize());
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUHandle(uint32_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = dxCommon_->GetSRVDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += (index * dxCommon_->GetSRVSize());
    return handle;
}