#include "Sprite.h"
#include <cassert>

Sprite::Sprite() {
    transform_ = InitializeWorldTransform();
    viewMatrix_ = MakeIdentity4x4();
    projectionMatrix_ = MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(1280), static_cast<float>(720), 0.0f, 100.0f);
    viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}

Sprite::~Sprite() {
    // リソースは ComPtr によって管理されるため、自動的に解放されます。
    // ただし、マップされている場合はアンマップすることが良い習慣です。
    if (vertexResource_ && vertexData_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (materialResource_ && materialData_) {
        materialResource_->Unmap(0, nullptr);
    }
    if (wvpResource_ && wvpData_) {
        wvpResource_->Unmap(0, nullptr);
    }
    if (indexResource_ && indexData_) {
        indexResource_->Unmap(0, nullptr);
    }
}


void Sprite::Initialize(Vector3 position, Color color, Vector3 anchor, Vector2 size, const std::string& textureFilePath) {
    transform_ = InitializeWorldTransform();

    // スプライトの幅と高さを定義
    float width = size.x;  // 現在の頂点データに基づく幅
    float height = size.y; // 現在の頂点データに基づく高さ

    // 頂点バッファの生成
    vertexResource_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), sizeof(VertexData) * 4);
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    // 頂点データの定義 (正方形)
    // anchorの概念を導入し、中心を原点とするように調整
    // 例: anchor = {0.5f, 0.5f, 0.0f} で中心を意味する
    float left = -width * anchor.x;
    float right = width * (1.0f - anchor.x);
    float top = -height * anchor.y;
    float bottom = height * (1.0f - anchor.y);

    // 左下
    vertexData_[0].position = { left, bottom, 0.0f, 1.0f };
    vertexData_[0].texcoord = { 0.0f, 1.0f };
    vertexData_[0].normal = { 0.0f, 0.0f, 1.0f };
    // 左上
    vertexData_[1].position = { left, top, 0.0f, 1.0f };
    vertexData_[1].texcoord = { 0.0f, 0.0f };
    vertexData_[1].normal = { 0.0f, 0.0f, 1.0f };
    // 右下
    vertexData_[2].position = { right, bottom, 0.0f, 1.0f };
    vertexData_[2].texcoord = { 1.0f, 1.0f };
    vertexData_[2].normal = { 0.0f, 0.0f, 1.0f };
    // 右上
    vertexData_[3].position = { right, top, 0.0f, 1.0f };
    vertexData_[3].texcoord = { 1.0f, 0.0f };
    vertexData_[3].normal = { 0.0f, 0.0f, 1.0f };

    // インデックスバッファの生成
    indexResource_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), sizeof(uint32_t) * 6);
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

    // インデックスデータの定義 (2つの三角形で正方形を構成)
    indexData_[0] = 0;
    indexData_[1] = 1;
    indexData_[2] = 2;
    indexData_[3] = 1;
    indexData_[4] = 3;
    indexData_[5] = 2;

    // マテリアルリソースの生成
    materialResource_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    // 引数で受け取ったcolorを設定
    materialData_->color = ToVector4(color);
    materialData_->enableLighting = false; // スプライトはライティングを無効にする
    materialData_->uvTransform = MakeIdentity4x4(); // UV変換を初期化

    // WVP リソースの生成
    wvpResource_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = MakeIdentity4x4(); // WVP 行列を初期化
    wvpData_->World = MakeIdentity4x4(); // ワールド行列を初期化

    // positionを設定
    transform_.translate = position;

    // テクスチャのロード
    textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/textures/" + textureFilePath);
}

void Sprite::Update(Vector3 spritePosition, Color color) {
    transform_.translate = spritePosition;

    // ワールド行列の計算
    wvpData_->World = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    // WVP 行列の計算
    Matrix4x4 worldViewProjectionMatrix = Multiply(wvpData_->World, viewProjectionMatrix_);
    wvpData_->WVP = worldViewProjectionMatrix;

    // マテリアルの更新
    materialData_->color = ToVector4(color); // enum Color を Vector4 に変換して設定

}

void Sprite::Draw() {
    // PSOの設定
    //DirectXCommon::GetInstance()->GetCommandList()->SetGraphicsRootSignature(DirectXCommon::GetInstance()->GetPSO()->GetRootSignature(PrimitiveType::kModel));
    //DirectXCommon::GetInstance()->GetCommandList()->SetPipelineState(DirectXCommon::GetInstance()->GetPSO()->GetPipelineState(PrimitiveType::kModel, BlendMode::kNormal));
    // インデックスバッファビューの設定
    DirectXCommon::GetInstance()->GetCommandList()->IASetIndexBuffer(&indexBufferView_);
    // 頂点バッファビューの設定
    DirectXCommon::GetInstance()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
    // プリミティブトポロジの設定 (三角形リスト)
    DirectXCommon::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // マテリアルCBufferの場所を設定
    DirectXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    // WVP CBufferの場所を設定
    DirectXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
    // テクスチャのDescriptorTableを設定
    DirectXCommon::GetInstance()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

    // 描画コマンド
    DirectXCommon::GetInstance()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0); // 6つのインデックス (2つの三角形) を描画
}