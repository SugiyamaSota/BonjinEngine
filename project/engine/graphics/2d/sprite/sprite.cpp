#include "Sprite.h"
#include <cassert>

namespace Bonjin {

    Sprite::Sprite()
    {
        viewMatrix_ = MakeIdentity4x4();
        // 画面解像度に合わせて射影行列を作成
        projectionMatrix_ = MakeOrthographicMatrix(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 100.0f);
        viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

        // 初期値設定
        anchor_ = { 0.0f, 0.0f, 0.0f };
        size_ = { 100.0f, 100.0f }; // デフォルトサイズ
        scale_ = { 1.0f, 1.0f };
        rotate_ = { 0.0f, 0.0f };
        translate_ = { 0.0f, 0.0f };
        color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

        dxCommon_ = DirectXCommon::GetInstance();
        device_ = dxCommon_->GetDevice();

        pso_ = dxCommon_->GetPSO()->GetPipelineState(
            device_, PrimitiveType::kModel, BlendMode::kNormal,
            D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK
        );
    }

    Sprite::~Sprite()
    {
        // Unmap処理
        if (vertexResource_) vertexResource_->Unmap(0, nullptr);
        if (materialResource_) materialResource_->Unmap(0, nullptr);
        if (wvpResource_) wvpResource_->Unmap(0, nullptr);
        if (indexResource_) indexResource_->Unmap(0, nullptr);
    }

    void Sprite::Initialize(const std::string& textureFilePath) {
        // --- 1. リソースの生成 (一生に一度だけ) ---

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

        // インデックスは不変なのでここで固定
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

        // テクスチャロード
        textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/textures/" + textureFilePath);

        // 初回の頂点情報を構築
        RefreshVertexData();
    }

    void Sprite::Update() {
        // 頂点座標とUVの更新 (ポインタ書き換えのみ)
        RefreshVertexData();

        // ワールド行列計算
        Vector3 scale = { scale_.x, scale_.y,0.f };
        Vector3 rotate = { rotate_.x,rotate_.y,0.f };
        Vector3 translate = { translate_.x,translate_.y,0.f };

        // ワールド行列の計算
        Matrix4x4 worldMatrix = MakeAffineMatrix(scale, rotate, translate);

        wvpData_->World = worldMatrix;
        wvpData_->WVP = Multiply(worldMatrix, viewProjectionMatrix_);
        materialData_->color = color_;

        UpdateUVTransform();
    }

    void Sprite::RefreshVertexData() {
        // テクスチャサイズの取得 (切り取り計算用)
        const D3D12_RESOURCE_DESC& resDesc = TextureManager::GetInstance()->GetResourceDesc(textureHandle_);
        float texW = static_cast<float>(resDesc.Width);
        float texH = static_cast<float>(resDesc.Height);

        // UVの決定
        float u_left = 0.0f, u_right = 1.0f, v_top = 0.0f, v_bottom = 1.0f;
        if (textureRect_.z != 0 && textureRect_.w != 0) {
            u_left = textureRect_.x / texW;
            v_top = textureRect_.y / texH;
            u_right = (textureRect_.x + textureRect_.z) / texW;
            v_bottom = (textureRect_.y + textureRect_.w) / texH;
        }

        // アンカーを考慮した座標計算
        float left = -size_.x * anchor_.x;
        float right = size_.x * (1.0f - anchor_.x);
        float top = -size_.y * anchor_.y;
        float bottom = size_.y * (1.0f - anchor_.y);

        // 頂点データの更新 (CreateBufferResourceを呼ばず、Map済みのポインタへ直接代入)
        // 0:左下, 1:左上, 2:右下, 3:右上
        vertexData_[0] = { {left, bottom, 0.0f, 1.0f}, {u_left, v_bottom}, {0,0,-1} };
        vertexData_[1] = { {left, top, 0.0f, 1.0f},    {u_left, v_top},    {0,0,-1} };
        vertexData_[2] = { {right, bottom, 0.0f, 1.0f}, {u_right, v_bottom}, {0,0,-1} };
        vertexData_[3] = { {right, top, 0.0f, 1.0f},    {u_right, v_top},    {0,0,-1} };
    }

    void Sprite::Draw() {
        auto commandList = dxCommon_->GetCommandList();

        commandList->SetGraphicsRootSignature(dxCommon_->GetPSO()->GetRootSignature(PrimitiveType::kModel));
        commandList->SetPipelineState(pso_);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->IASetIndexBuffer(&indexBufferView_);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
    }

    void Sprite::UpdateUVTransform() {
        // 反転フラグを反映させたUV変形行列を作成
        Vector3 flipScale = { isFlipX_ ? -1.0f : 1.0f, isFlipY_ ? -1.0f : 1.0f, 1.0f };
        Vector3 flipTranslate = { isFlipX_ ? 1.0f : 0.0f, isFlipY_ ? 1.0f : 0.0f, 0.0f };

        materialData_->uvTransform = MakeAffineMatrix(flipScale, Vector3{ 0,0,0 }, flipTranslate);
    }
}