#pragma once
#include <d3d12.h>

/// <summary>
/// ポリゴンの塗りつぶしモード（DirectX12非依存）
/// </summary>
enum class FillMode {
    kSolid,     // 通常の塗りつぶし
    kWireframe, // ワイヤーフレーム表示
};

/// <summary>
/// カリングモード（裏面非描画設定、DirectX12非依存）
/// </summary>
enum class CullMode {
    kNone,  // カリングなし（両面描画）
    kFront, // 前面カリング
    kBack,  // 背面カリング（標準）
};

/// <summary>
/// FillMode から Direct3D12 の D3D12_FILL_MODE への変換
/// </summary>
inline D3D12_FILL_MODE ToD3D12(FillMode mode) {
    switch (mode) {
    case FillMode::kWireframe:
        return D3D12_FILL_MODE_WIREFRAME;
    case FillMode::kSolid:
    default:
        return D3D12_FILL_MODE_SOLID;
    }
}

/// <summary>
/// CullMode から Direct3D12 の D3D12_CULL_MODE への変換
/// </summary>
inline D3D12_CULL_MODE ToD3D12(CullMode mode) {
    switch (mode) {
    case CullMode::kNone:
        return D3D12_CULL_MODE_NONE;
    case CullMode::kFront:
        return D3D12_CULL_MODE_FRONT;
    case CullMode::kBack:
    default:
        return D3D12_CULL_MODE_BACK;
    }
}
