#pragma once
#include "DirectXCommon.h"
#include <vector>

enum class SrvType {
    Texture2D,
    TextureCube,
    StructuredBuffer
};

class SrvManager {
public:
    static SrvManager* GetInstance();
    void Initialize();

    // 空いているSRVのインデックスを確保
    uint32_t Allocate();

    void CreateSrv(uint32_t index, ID3D12Resource* resource, SrvType type, uint32_t numElements = 0, uint32_t stride = 0);

    // 描画前のセット処理
    void PreDraw();

    // インデックスからハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index);

private:

    uint32_t useCount_ = 0; // 現在どこまで使っているか
    const uint32_t kMaxSRVCount = 512; // 最大数
};