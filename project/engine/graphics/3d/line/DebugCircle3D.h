#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "Struct.h"

class Camera;
class DirectXCommon;

namespace Bonjin {

class DebugCircle3D {
public:
    DebugCircle3D();
    ~DebugCircle3D();

    void Initialize();
    void Update(const Vector3& center, float radius, const Camera* camera, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
    void Draw();

private:
    struct LineVertex {
        Vector4 position;
        Vector4 color;
    };

    static inline const int kSubdivisions = 32;
    static inline const int kVertexCount = kSubdivisions * 2;

    DirectXCommon* dxCommon_ = nullptr;
    ID3D12Device* device_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> matrixResource_;
    
    LineVertex* vertexData_ = nullptr;
    Matrix4x4* matrixData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Vector4 color_{ 1.0f, 1.0f, 1.0f, 1.0f };
};

}
