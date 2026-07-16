#include "DebugCircle3D.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "function/function.h"
#include "PSOManager.h"
#include "Matrix.h"
#include <cmath>

namespace Bonjin {

DebugCircle3D::DebugCircle3D() {
    dxCommon_ = DirectXCommon::GetInstance();
    device_ = dxCommon_->GetDevice();
}

DebugCircle3D::~DebugCircle3D() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (matrixResource_) {
        matrixResource_->Unmap(0, nullptr);
    }
}

void DebugCircle3D::Initialize() {
    // kVertexCount 個の頂点リソースを確保
    vertexResource_ = CreateBufferResource(device_, sizeof(LineVertex) * kVertexCount);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(LineVertex) * kVertexCount;
    vertexBufferView_.StrideInBytes = sizeof(LineVertex);

    // 行列用のリソースを確保
    matrixResource_ = CreateBufferResource(device_, sizeof(Matrix4x4));
    matrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&matrixData_));
    *matrixData_ = MakeIdentity4x4();
}

void DebugCircle3D::Update(const Vector3& center, float radius, const Camera* camera, const Vector4& color) {
    color_ = color;

    // XY平面上の円を形成する頂点データを書き込む
    float angleStep = (2.0f * 3.14159265359f) / kSubdivisions;
    int vertexIndex = 0;
    
    // Z座標を少し手前にする (-0.1f)
    float zOffset = -0.1f;

    for (int i = 0; i < kSubdivisions; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        Vector3 p1 = {
            center.x + std::cos(angle1) * radius,
            center.y + std::sin(angle1) * radius,
            center.z + zOffset
        };
        Vector3 p2 = {
            center.x + std::cos(angle2) * radius,
            center.y + std::sin(angle2) * radius,
            center.z + zOffset
        };

        // 線のスタートとエンド
        vertexData_[vertexIndex++] = { {p1.x, p1.y, p1.z, 1.0f}, color_ };
        vertexData_[vertexIndex++] = { {p2.x, p2.y, p2.z, 1.0f}, color_ };
    }

    // WVP行列書き込み (ViewProjection)
    *matrixData_ = camera->GetViewProjectionMatrix();
}

void DebugCircle3D::Draw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    PSOManager* psoManager = dxCommon_->GetPSO();
    
    ID3D12PipelineState* pipelineState = psoManager->GetPipelineState(
        device_,
        PrimitiveType::kLine,
        BlendMode::kNormal,
        D3D12_FILL_MODE_SOLID,
        D3D12_CULL_MODE_NONE);

    commandList->SetGraphicsRootSignature(psoManager->GetRootSignature(PrimitiveType::kLine));
    commandList->SetPipelineState(pipelineState);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList->SetGraphicsRootConstantBufferView(0, matrixResource_->GetGPUVirtualAddress());
    commandList->DrawInstanced(kVertexCount, 1, 0, 0);
}

}
