#include "FrayLine3D.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "function/function.h"
#include "PSOManager.h"
#include "Matrix.h"
#include "vector.h"
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <algorithm>

namespace Bonjin {

FrayLine3D::FrayLine3D() {
    dxCommon_ = DirectXCommon::GetInstance();
    device_ = dxCommon_->GetDevice();
}

FrayLine3D::~FrayLine3D() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (matrixResource_) {
        matrixResource_->Unmap(0, nullptr);
    }
}

void FrayLine3D::Initialize() {
    vertexResource_ = CreateBufferResource(device_, sizeof(LineVertex) * kVertexCount);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(LineVertex) * kVertexCount;
    vertexBufferView_.StrideInBytes = sizeof(LineVertex);

    matrixResource_ = CreateBufferResource(device_, sizeof(Matrix4x4));
    matrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&matrixData_));
    *matrixData_ = MakeIdentity4x4();
}

void FrayLine3D::Update(
    const Vector3& start,
    const Vector3& end,
    const Camera* camera,
    const Vector4& color,
    float progress,
    float tension)
{
    animTime_ += 1.0f / 60.0f;

    Vector3 dir = Subtract(end, start);
    float length = Length(dir);
    if (length < 0.001f || (progress >= 1.0f && tension <= 0.0f)) {
        // 長さがゼロまたは消滅完了時は透明化して終了
        for (int i = 0; i < kVertexCount; ++i) {
            vertexData_[i] = { {start.x, start.y, start.z, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f} };
        }
        *matrixData_ = camera->GetViewProjectionMatrix();
        return;
    }

    Vector3 dirNorm = Normalize(dir);

    // 糸の横方向（法線）ベクトル
    Vector3 right = { -dirNorm.y, dirNorm.x, 0.0f };
    if (Length(right) < 0.001f) {
        right = { 0.0f, 1.0f, 0.0f };
    } else {
        right = Normalize(right);
    }
    Vector3 up = { 0.0f, 0.0f, 1.0f };

    int vertexIndex = 0;

    if (progress > 0.0f) {
        // =========================================================
        // 【切断・ほつれアニメーション中】
        // =========================================================
        float fadeAlpha = (std::clamp)(1.0f - progress * progress, 0.0f, 1.0f);
        Vector4 drawColor = color;
        drawColor.w *= fadeAlpha;

        // 糸が中央から切れて両端へ縮む比率 (0.5 -> 0.0)
        float halfRemaining = 0.5f * (1.0f - progress);

        // 繊維ごとに描画 (kNumStrands = 3)
        for (int strand = 0; strand < kNumStrands; ++strand) {
            // 各繊維ごとの位相ずれ
            float strandPhase = (float)strand * (std::numbers::pi_v<float> * 2.0f / kNumStrands);
            float strandCurlDir = (strand % 2 == 0) ? 1.0f : -1.0f;

            // 半分（手元側）と半分（アンカー側）に分けて各 (kSubdivisions / 2) セグメント
            int halfSubs = kSubdivisions / 2;

            // 1. プレイヤー側から中央へ伸びるほつれ糸
            Vector3 curPosA = start;
            for (int i = 0; i < halfSubs; ++i) {
                float tNext = (float)(i + 1) / halfSubs * halfRemaining; // 0.0 -> halfRemaining
                Vector3 basePos = Add(start, Multiply(tNext, dir));

                // 切れた先端（tNextが大きいほど）大きくカール・たわむ
                float frayWeight = (float)(i + 1) / halfSubs; // 0.0 -> 1.0
                float wave = std::sin(tNext * 18.0f + animTime_ * 12.0f + strandPhase) * 0.45f * frayWeight;
                float curl = strandCurlDir * progress * 0.6f * frayWeight;

                Vector3 offset = Add(Multiply(wave + curl, right), Multiply(std::cos(tNext * 12.0f) * 0.2f * frayWeight, up));
                Vector3 nextPos = Add(basePos, offset);

                vertexData_[vertexIndex++] = { {curPosA.x, curPosA.y, curPosA.z, 1.0f}, drawColor };
                vertexData_[vertexIndex++] = { {nextPos.x, nextPos.y, nextPos.z, 1.0f}, drawColor };

                curPosA = nextPos;
            }

            // 2. アンカー側から中央へ伸びるほつれ糸
            Vector3 curPosB = end;
            for (int i = 0; i < halfSubs; ++i) {
                float tNext = (float)(i + 1) / halfSubs * halfRemaining;
                Vector3 basePos = Subtract(end, Multiply(tNext, dir));

                float frayWeight = (float)(i + 1) / halfSubs;
                float wave = std::sin(tNext * 18.0f - animTime_ * 12.0f + strandPhase + 1.0f) * 0.45f * frayWeight;
                float curl = -strandCurlDir * progress * 0.6f * frayWeight;

                Vector3 offset = Add(Multiply(wave + curl, right), Multiply(std::sin(tNext * 12.0f) * 0.2f * frayWeight, up));
                Vector3 nextPos = Add(basePos, offset);

                vertexData_[vertexIndex++] = { {curPosB.x, curPosB.y, curPosB.z, 1.0f}, drawColor };
                vertexData_[vertexIndex++] = { {nextPos.x, nextPos.y, nextPos.z, 1.0f}, drawColor };

                curPosB = nextPos;
            }
        }
    } else {
        // =========================================================
        // 【長押しチャージ中 / テンション振動中】
        // =========================================================
        // テンションに応じた白発光ブレンド
        Vector4 whiteColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        Vector4 drawColor = {
            color.x + (whiteColor.x - color.x) * (tension * 0.7f),
            color.y + (whiteColor.y - color.y) * (tension * 0.7f),
            color.z + (whiteColor.z - color.z) * (tension * 0.7f),
            (std::min)(1.0f, color.w + tension * 0.4f)
        };

        // テンション時の高周波ジッター幅
        float jitterAmp = tension * 0.12f;

        for (int strand = 0; strand < kNumStrands; ++strand) {
            float strandPhase = (float)strand * 1.5f;
            Vector3 curPos = start;

            for (int i = 0; i < kSubdivisions; ++i) {
                float tNext = (float)(i + 1) / kSubdivisions;
                Vector3 nextPos;

                if (i == kSubdivisions - 1) {
                    nextPos = end;
                } else {
                    Vector3 basePos = Add(start, Multiply(tNext, dir));

                    // 中央付近ほど揺れやすい山なり形状
                    float envelope = std::sin(tNext * std::numbers::pi_v<float>);
                    // 高速振動サイン波 + ランダムジッター
                    float fastWave = std::sin(tNext * 25.0f + animTime_ * 40.0f + strandPhase) * jitterAmp * envelope;
                    float randJitter = ((float)std::rand() / RAND_MAX * 2.0f - 1.0f) * (jitterAmp * 0.5f) * envelope;

                    Vector3 offset = Multiply(fastWave + randJitter, right);
                    nextPos = Add(basePos, offset);
                }

                vertexData_[vertexIndex++] = { {curPos.x, curPos.y, curPos.z, 1.0f}, drawColor };
                vertexData_[vertexIndex++] = { {nextPos.x, nextPos.y, nextPos.z, 1.0f}, drawColor };

                curPos = nextPos;
            }
        }
    }

    // 残りの頂点を透明化で安全に埋める
    while (vertexIndex < kVertexCount) {
        vertexData_[vertexIndex++] = { {start.x, start.y, start.z, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f} };
    }

    *matrixData_ = camera->GetViewProjectionMatrix();
}

void FrayLine3D::Draw() {
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
