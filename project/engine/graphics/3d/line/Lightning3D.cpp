#include "Lightning3D.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "function/function.h"
#include "PSOManager.h"
#include "Matrix.h"
#include "vector.h"
#include <cmath>
#include <cstdlib>

namespace Bonjin {

Lightning3D::Lightning3D() {
    dxCommon_ = DirectXCommon::GetInstance();
    device_ = dxCommon_->GetDevice();
}

Lightning3D::~Lightning3D() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (matrixResource_) {
        matrixResource_->Unmap(0, nullptr);
    }
}

void Lightning3D::Initialize() {
    vertexResource_ = CreateBufferResource(device_, sizeof(LineVertex) * kVertexCount);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(LineVertex) * kVertexCount;
    vertexBufferView_.StrideInBytes = sizeof(LineVertex);

    matrixResource_ = CreateBufferResource(device_, sizeof(Matrix4x4));
    matrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&matrixData_));
    *matrixData_ = MakeIdentity4x4();
}

void Lightning3D::Update(const Vector3& start, const Vector3& end, const Camera* camera, const Vector4& color, float offsetRatio, float minOffset, float maxOffsetLimit) {
    color_ = color;

    // 開始から終了へのベクトル
    Vector3 dir = Subtract(end, start);
    float length = Length(dir);
    if (length < 0.001f) {
        // 長さが極小の場合は、何も描画しないように頂点色を透明にするか、縮退させる
        for (int i = 0; i < kVertexCount; ++i) {
            vertexData_[i] = { {start.x, start.y, start.z, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f} };
        }
        *matrixData_ = camera->GetViewProjectionMatrix();
        return;
    }

    Vector3 dirNorm = Normalize(dir);

    // 稲妻の法線ベクトル方向 (XY平面での垂直ベクトル + Z軸方向)
    Vector3 right = { -dirNorm.y, dirNorm.x, 0.0f };
    if (Length(right) < 0.001f) {
        right = { 0.0f, 1.0f, 0.0f };
    } else {
        right = Normalize(right);
    }
    Vector3 up = { 0.0f, 0.0f, 1.0f };

    int vertexIndex = 0;

    for (int line = 0; line < kNumLines; ++line) {
        Vector3 currentPos = start;

        for (int i = 0; i < kSubdivisions; ++i) {
            float t1 = (float)i / kSubdivisions;
            float t2 = (float)(i + 1) / kSubdivisions;

            Vector3 nextPos;
            if (i == kSubdivisions - 1) {
                nextPos = end;
            } else {
                // 線形補間位置
                Vector3 basePos = Add(start, Multiply(t2, dir));
                
                // 揺らぎ幅 (ベクトルの長さの比率を基準にする)
                float maxOffset = length * offsetRatio;
                // 揺らぎ幅が極端に大きく/小さくなりすぎないよう調整
                maxOffset = std::clamp(maxOffset, minOffset, maxOffsetLimit);

                // ランダムノイズ
                float randOffsetRight = ((float)std::rand() / RAND_MAX * 2.0f - 1.0f) * maxOffset;
                float randOffsetUp = ((float)std::rand() / RAND_MAX * 2.0f - 1.0f) * maxOffset * 0.4f;

                nextPos = Add(basePos, Add(Multiply(randOffsetRight, right), Multiply(randOffsetUp, up)));
            }

            // 頂点にノイズを含めて線を描画
            vertexData_[vertexIndex++] = { {currentPos.x, currentPos.y, currentPos.z, 1.0f}, color_ };
            vertexData_[vertexIndex++] = { {nextPos.x, nextPos.y, nextPos.z, 1.0f}, color_ };

            currentPos = nextPos;
        }
    }

    *matrixData_ = camera->GetViewProjectionMatrix();
}

void Lightning3D::Draw() {
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
