#include "BaseSprite.h"
#include "function/function.h"
#include <cassert>

namespace Bonjin {

    BaseSprite::BaseSprite()
    {
        viewMatrix_ = MakeIdentity4x4();
        projectionMatrix_ = MakeOrthographicMatrix(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 100.0f);
        viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

        // デフォルトパラメータ
        anchor_ = { 0.0f, 0.0f, 0.0f };
        size_ = { 100.0f, 100.0f };
        scale_ = { 1.0f, 1.0f };
        rotate_ = { 0.0f, 0.0f };
        translate_ = { 0.0f, 0.0f };
        color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

        dxCommon_ = DirectXCommon::GetInstance();
        device_ = dxCommon_->GetDevice();

        pso_ = dxCommon_->GetPSO()->GetPipelineState(
            device_, PrimitiveType::kSprite, BlendMode::kNormal,
            D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE
        );
    }

    BaseSprite::~BaseSprite()
    {
        if (vertexResource_) vertexResource_->Unmap(0, nullptr);
        if (materialResource_) materialResource_->Unmap(0, nullptr);
        if (wvpResource_) wvpResource_->Unmap(0, nullptr);
        if (indexResource_) indexResource_->Unmap(0, nullptr);
    }

    void BaseSprite::Initialize() {
        // 頂点バッファ
        vertexResource_ = CreateBufferResource(device_, sizeof(VertexData) * 4);
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
        vertexBufferView_.StrideInBytes = sizeof(VertexData);
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

        // インデックスバッファ
        indexResource_ = CreateBufferResource(device_, sizeof(uint32_t) * 6);
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

        indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
        indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;

        // マテリアル
        materialResource_ = CreateBufferResource(device_, sizeof(Material));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
        materialData_->enableLighting = false;
        materialData_->uvTransform = MakeIdentity4x4();

        // WVP
        wvpResource_ = CreateBufferResource(device_, sizeof(TransformationMatrix));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));

        RefreshVertexData();
    }

    void BaseSprite::Update() {
        RefreshVertexData();

        Vector3 scale = { scale_.x, scale_.y, 0.0f };
        Vector3 rotate = { rotate_.x, rotate_.y, 0.0f };
        Vector3 translate = { translate_.x, translate_.y, 0.0f };

        Matrix4x4 worldMatrix = MakeAffineMatrix(scale, rotate, translate);

        wvpData_->World = worldMatrix;
        wvpData_->WVP = Multiply(worldMatrix, viewProjectionMatrix_);
        materialData_->color = color_;

        UpdateUVTransform();
    }

    void BaseSprite::RefreshVertexData() {
        float left = -size_.x * anchor_.x;
        float right = size_.x * (1.0f - anchor_.x);
        float top = -size_.y * anchor_.y;
        float bottom = size_.y * (1.0f - anchor_.y);

        // 0:左下, 1:左上, 2:右下, 3:右上 (全面描画のデフォルトUV)
        vertexData_[0] = { {left, bottom, 0.0f, 1.0f}, {0.0f, 1.0f}, {0,0,-1} };
        vertexData_[1] = { {left, top, 0.0f, 1.0f},    {0.0f, 0.0f}, {0,0,-1} };
        vertexData_[2] = { {right, bottom, 0.0f, 1.0f}, {1.0f, 1.0f}, {0,0,-1} };
        vertexData_[3] = { {right, top, 0.0f, 1.0f},    {1.0f, 0.0f}, {0,0,-1} };
    }

    void BaseSprite::UpdateUVTransform() {
        Vector3 flipScale = { isFlipX_ ? -1.0f : 1.0f, isFlipY_ ? -1.0f : 1.0f, 1.0f };
        Vector3 flipTranslate = { isFlipX_ ? 1.0f : 0.0f, isFlipY_ ? 1.0f : 0.0f, 0.0f };

        materialData_->uvTransform = MakeAffineMatrix(flipScale, Vector3{ 0,0,0 }, flipTranslate);
    }

    void BaseSprite::Draw() {
        auto commandList = dxCommon_->GetCommandList();

        commandList->SetGraphicsRootSignature(dxCommon_->GetPSO()->GetRootSignature(PrimitiveType::kSprite));
        commandList->SetPipelineState(pso_);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->IASetIndexBuffer(&indexBufferView_);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
        
        // 仮想関数経由で適切なSRVを取得してバインド
        commandList->SetGraphicsRootDescriptorTable(2, GetSRVHandle());

        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
    }

}
