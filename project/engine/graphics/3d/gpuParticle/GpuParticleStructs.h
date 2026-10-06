#pragma once
#include <d3d12.h>
#include "Struct.h"
#include "Vector.h"
#include "Matrix.h"
#include <cstdint>

namespace Bonjin {

// HLSL側の ParticleDataGPU と完全に一致するアラインメント
struct alignas(16) ParticleDataGPU {
    Vector3 position{ 0.0f, 0.0f, 0.0f };
    float scale = 0.0f;
    Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    float lifeRatio = 1.0f; // 1.0以上で死亡
    uint32_t particleType = 0; // 0: 雷ボルト, 1: スパーク火花, 2: 接地点グロー
    float customParam = 0.0f;
};

// シーン全体情報
struct alignas(16) GpuSceneBuffer {
    Matrix4x4 viewProjection;
    Matrix4x4 view;
    Matrix4x4 billboardMatrix;
    Vector3 cameraPosition;
    float time = 0.0f;
};

// 頂点バッファ用構造体 (ビルボード板ポリゴン用)
struct GpuParticleVertex {
    Vector4 position;
    Vector2 texcoord;
    Vector4 color;
};

// 内部パーティクル状態
struct ParticleState {
    Vector3 position{ 0.0f, 0.0f, 0.0f };
    Vector3 velocity{ 0.0f, 0.0f, 0.0f };
    Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    float scale = 0.2f;
    float baseScale = 0.2f;
    float lifeTime = 0.2f;
    float currentLife = 0.0f;
    float intensity = 2.0f;
    uint32_t type = 0; // 0: 雷ボルト, 1: スパーク, 2: グロー
    bool isActive = false;
};

// アプリ層から指定する雷生成設定
struct GpuLightningConfig {
    Vector3 start{ 0.0f, 0.0f, 0.0f };
    Vector3 end{ 0.0f, 5.0f, 0.0f };
    Vector4 coreColor{ 1.0f, 1.0f, 1.0f, 1.0f };     // 芯の色（純白推奨）
    Vector4 glowColor{ 0.25f, 0.65f, 1.0f, 1.0f };   // 外周の電撃色（シアン/青/紫）
    float boltRadius = 0.35f;                        // 稲妻の揺らぎ・ギザギザ幅
    float boltScale = 0.22f;                         // 雷ボルトの太さ
    int subdivisions = 7;                            // 分割深度 (2^7 = 128セグメント)
    int mainBranches = 3;                            // 分岐枝の数
    int sparksPerSegment = 2;                        // 経路から飛散するスパーク数
    int impactSparks = 80;                           // 着弾地点で飛び散る火花数
    float duration = 0.16f;                          // 発光持続時間（秒）
    float intensity = 3.5f;                          // 発光強度 (Bloom加算用)
    float sparkSpeed = 7.0f;                         // 火花の初速
};

} // namespace Bonjin
