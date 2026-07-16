#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "Struct.h"

class Camera;
class DirectXCommon;

namespace Bonjin {

class Lightning3D {
public:
    Lightning3D();
    ~Lightning3D();

    void Initialize();
    void Update(const Vector3& start, const Vector3& end, const Camera* camera, const Vector4& color = { 0.3f, 0.6f, 1.0f, 1.0f }, float offsetRatio = 0.08f, float minOffset = 0.2f, float maxOffsetLimit = 1.5f);
    void Draw();

private:
    struct LineVertex {
        Vector4 position;
        Vector4 color;
    };

    static constexpr int kNumLines = 3;       // 稲妻の数
    static constexpr int kSubdivisions = 6;   // 1本の分割数
    static constexpr int kVertexCount = kNumLines * kSubdivisions * 2;

    DirectXCommon* dxCommon_ = nullptr;
    ID3D12Device* device_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> matrixResource_;
    
    LineVertex* vertexData_ = nullptr;
    Matrix4x4* matrixData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Vector4 color_{ 0.3f, 0.6f, 1.0f, 1.0f };
};

}
