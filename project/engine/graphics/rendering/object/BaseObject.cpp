#include "BaseObject.h"
#include <cassert>

// コンストラクタで基本的なインスタンスを取得
BaseObject::BaseObject() {
    common_ = DirectXCommon::GetInstance();
    device_ = common_->GetDevice();
}

void BaseObject::ReleaseResources() {
    // 頂点リソースの解放
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
        vertexResource_.Reset();
    }
    // マテリアルリソースの解放
    if (materialResource_) {
        materialResource_->Unmap(0, nullptr);
        materialResource_.Reset();
    }

    vertexData_ = nullptr;
    materialData_ = nullptr;
}

void BaseObject::LoadModel(const std::string& directoryName, const std::string& fileName) {
    const std::string directoryPath = "resources/models/" + directoryName;
    const std::string objFilename = fileName;

    // ModelManagerから取得
    modelData_ = ModelManager::GetInstance()->LoadModel(directoryPath, objFilename);

    // リソースのセットアップ
	CreateVertexResource(modelData_.vertices);
	CreateMaterialResource();

	SetupResources();

    // テクスチャのロード (ファイルモデル特有)
    textureHandle_ = TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilepath);
    common_->WaitAndResetCommandList();
    TextureManager::GetInstance()->ReleaseIntermediateResources();
}


void BaseObject::CreateModel(ModelBuilder::ModelType type, const std::string& textureFilepath) {
    modelData_ = ModelBuilder::CreateModel(type);

    // 基底クラスのメソッドを利用してリソース作成
    CreateVertexResource(modelData_.vertices);
    CreateMaterialResource();

    // Object3D特有のリソース作成
    SetupResources();

    textureHandle_ = TextureManager::GetInstance()->LoadTexture(textureFilepath);
    common_->WaitAndResetCommandList();
    TextureManager::GetInstance()->ReleaseIntermediateResources();
}


void BaseObject::CreateVertexResource(const std::vector<VertexData>& vertices) {
    // 頂点バッファの生成
    vertexResource_ = CreateBufferResource(device_, sizeof(VertexData) * vertices.size());

    // ビューの設定
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // データのマッピングとコピー
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    std::memcpy(vertexData_, vertices.data(), sizeof(VertexData) * vertices.size());
}

void BaseObject::CreateMaterialResource() {
    // マテリアルバッファの生成
    materialResource_ = CreateBufferResource(device_, sizeof(Material));

    // データのマッピングと初期値設定
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData_->enableLighting = true;
    materialData_->uvTransform = MakeIdentity4x4();
}