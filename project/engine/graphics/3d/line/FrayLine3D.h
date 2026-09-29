#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "Struct.h"

class Camera;
class DirectXCommon;

namespace Bonjin {

/// <summary>
/// アンカーの糸のテンション振動・ほつれ切断エフェクトを描画するクラス
/// </summary>
class FrayLine3D {
public:
    FrayLine3D();
    ~FrayLine3D();

    void Initialize();

    /// <summary>
    /// ほつれ・切断エフェクトの更新
    /// </summary>
    /// <param name="start">プレイヤー手元座標</param>
    /// <param name="end">アンカー座標</param>
    /// <param name="camera">カメラ</param>
    /// <param name="color">ベース色</param>
    /// <param name="progress">ほつれ進行度 (0.0f: 切断直後 ~ 1.0f: 完全消滅)</param>
    /// <param name="tension">長押しテンション (0.0f: 通常 ~ 1.0f: 限界発光・振動)</param>
    void Update(
        const Vector3& start,
        const Vector3& end,
        const Camera* camera,
        const Vector4& color,
        float progress,
        float tension = 0.0f
    );

    void Draw();

private:
    struct LineVertex {
        Vector4 position;
        Vector4 color;
    };

    static constexpr int kNumStrands = 3;       // ほつれた繊維の本数
    static constexpr int kSubdivisions = 10;     // 1本の繊維の分割数
    // 各繊維につき (kSubdivisions * 2) 頂点
    static constexpr int kVertexCount = kNumStrands * kSubdivisions * 2;

    DirectXCommon* dxCommon_ = nullptr;
    ID3D12Device* device_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> matrixResource_;

    LineVertex* vertexData_ = nullptr;
    Matrix4x4* matrixData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Vector4 color_{ 0.5f, 0.85f, 1.0f, 0.5f };
    float animTime_ = 0.0f;
};

}
