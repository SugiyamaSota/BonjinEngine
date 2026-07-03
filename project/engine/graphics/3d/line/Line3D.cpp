#include "Line3D.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "function/function.h"
#include "PSOManager.h"
#include "Matrix.h"

namespace Bonjin {

Line3D::Line3D() {
    dxCommon_ = DirectXCommon::GetInstance();
    device_ = dxCommon_->GetDevice();
}

Line3D::~Line3D() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (matrixResource_) {
        matrixResource_->Unmap(0, nullptr);
    }
}

void Line3D::Initialize() {
    // 2頂点分のリソースを確保
    vertexResource_ = CreateBufferResource(device_, sizeof(LineVertex) * 2);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(LineVertex) * 2;
    vertexBufferView_.StrideInBytes = sizeof(LineVertex);

    // 行列用のリソースを確保
    matrixResource_ = CreateBufferResource(device_, sizeof(Matrix4x4));
    matrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&matrixData_));
    *matrixData_ = MakeIdentity4x4();
}

void Line3D::Update(const Vector3& start, const Vector3& end, const Camera* camera, const Vector4& color) {
    color_ = color;

    // 頂点データ書き込み
    vertexData_[0] = { {start.x, start.y, start.z, 1.0f}, color_ };
    vertexData_[1] = { {end.x, end.y, end.z, 1.0f}, color_ };

    // WVP行列書き込み (Line3Dはワールド座標系で描画するため、Worldは単位行列。よって WVP = ViewProjection)
    *matrixData_ = camera->GetViewProjectionMatrix();
}

void Line3D::Draw() {
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
    commandList->DrawInstanced(2, 1, 0, 0);
}

}
