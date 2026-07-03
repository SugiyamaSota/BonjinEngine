#include "Line2D.h"
#include "DirectXCommon.h"
#include "function/function.h"
#include "PSOManager.h"
#include "Convert.h"
#include "Matrix.h"

namespace Bonjin {

Line2D::Line2D() {
    dxCommon_ = DirectXCommon::GetInstance();
    device_ = dxCommon_->GetDevice();
    // 既存のスプライトに合わせ、1280x720の正射影行列を作成
    projectionMatrix_ = MakeOrthographicMatrix(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 100.0f);
}

Line2D::~Line2D() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (matrixResource_) {
        matrixResource_->Unmap(0, nullptr);
    }
}

void Line2D::Initialize() {
    // 2頂点分のリソースを確保
    vertexResource_ = CreateBufferResource(device_, sizeof(LineVertex) * 2);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(LineVertex) * 2;
    vertexBufferView_.StrideInBytes = sizeof(LineVertex);

    // 行列用のリソースを確保
    matrixResource_ = CreateBufferResource(device_, sizeof(Matrix4x4));
    matrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&matrixData_));
    *matrixData_ = projectionMatrix_;
}

void Line2D::Update(const Vector2& start, const Vector2& end, const Vector4& color) {
    color_ = color;

    // 頂点データ書き込み
    vertexData_[0] = { {start.x, start.y, 0.0f, 1.0f}, color_ };
    vertexData_[1] = { {end.x, end.y, 0.0f, 1.0f}, color_ };

    // WVP行列書き込み (Line2Dは2D描画のため、WorldもViewも単位行列、WVP = Projection)
    *matrixData_ = projectionMatrix_;
}

void Line2D::Draw() {
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
