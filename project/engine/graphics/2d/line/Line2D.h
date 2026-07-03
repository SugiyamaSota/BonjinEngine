#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "Struct.h"

class DirectXCommon;

namespace Bonjin {

class Line2D {
public:
    Line2D();
    ~Line2D();

    void Initialize();
    void Update(const Vector2& start, const Vector2& end, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
    void Draw();

    void SetColor(const Vector4& color) { color_ = color; }

private:
    struct LineVertex {
        Vector4 position;
        Vector4 color;
    };

    DirectXCommon* dxCommon_ = nullptr;
    ID3D12Device* device_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> matrixResource_;
    
    LineVertex* vertexData_ = nullptr;
    Matrix4x4* matrixData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Vector4 color_{ 1.0f, 1.0f, 1.0f, 1.0f };
    Matrix4x4 projectionMatrix_;
};

}
