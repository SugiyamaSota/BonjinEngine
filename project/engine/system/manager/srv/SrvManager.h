#pragma once
#include "DirectXCommon.h"
#include <vector>

class SrvManager {
public:
    static SrvManager* GetInstance();
    void Initialize();

    // 空いているSRVのインデックスを確保
    uint32_t Allocate();

    // 指定したインデックスにテクスチャSRVを作成
    void CreateTextureSrv(uint32_t index, ID3D12Resource* resource);

    // 指定したインデックスに構造化バッファSRVを作成 (Particle用)
    void CreateStructuredBufferSrv(uint32_t index, ID3D12Resource* resource, uint32_t numElements, uint32_t structureByteStride);

    // 描画前のセット処理
    void PreDraw();

    // インデックスからハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index);

private:
    static SrvManager* instance_;
    DirectXCommon* dxCommon_ = nullptr;

    uint32_t useCount_ = 0; // 現在どこまで使っているか
    const uint32_t kMaxSRVCount = 512; // 最大数
};