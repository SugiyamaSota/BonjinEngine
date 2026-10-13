#include "BaseObject.h"
#include "../../../interface/IScene.h"
#include "SceneManager.h"
#include <cassert>

BaseObject::~BaseObject() {
	ReleaseResources();
	if (parentScene_) {
		parentScene_->UnregisterObject(this);
	}
}

// コンストラクタで基本的なインスタンスを取得
BaseObject::BaseObject() {
    common_ = DirectXCommon::GetInstance();
    device_ = common_->GetDevice();

	envTextureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/textures/skyBox.dds");

	parentScene_ = Bonjin::SceneManager::GetInstance()->GetCurrentScene();
	if (parentScene_) {
		parentScene_->RegisterObject(this);
	}
}

void BaseObject::ReleaseResources() {
    // 頂点・インデックスリソースの解放
    // 作成時にUnmap済みのため、Reset()で安全に参照カウントを減らす
    vertexResource_.Reset();
    indexResource_.Reset();
    vertexData_ = nullptr;

    // マテリアルリソースの解放（所有者のみUnmap）
    if (materialResource_) {
        if (isMaterialOwner_) {
            materialResource_->Unmap(0, nullptr);
        }
        materialResource_.Reset();
    }
    materialData_ = nullptr;
    isMaterialOwner_ = false;
    isResourceOwner_ = false;
}

void BaseObject::LoadModel(const std::string& directoryName, const std::string& fileName) {
    const std::string directoryPath = "resources/models/" + directoryName;
    const std::string objFilename = fileName;

    // ModelManagerから取得
    modelData_ = ModelManager::GetInstance()->LoadModel(directoryPath, objFilename);

    // リソースのセットアップ
	CreateVertexResource(modelData_.vertices);
	CreateIndexResource(modelData_.indices);
	CreateMaterialResource();

	SetupResources();

    // テクスチャのロード (ファイルモデル特有)
    if (!modelData_.material.textureFilepath.empty()) {
        bool isNew = !TextureManager::GetInstance()->IsLoaded(modelData_.material.textureFilepath);
        textureHandle_ = TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilepath);
        if (isNew) {
            common_->WaitAndResetCommandList();
            TextureManager::GetInstance()->ReleaseIntermediateResources();
        }
    }
}

void BaseObject::SetModel(const std::string& modelKey) {
    // ModelManagerからキー指定で取得
    modelData_ = ModelManager::GetInstance()->GetModel(modelKey);

    // リソースのセットアップ
    CreateVertexResource(modelData_.vertices);
    CreateIndexResource(modelData_.indices);
    CreateMaterialResource();

    SetupResources();

    // テクスチャのロード (ファイルモデル特有)
    if (!modelData_.material.textureFilepath.empty()) {
        bool isNew = !TextureManager::GetInstance()->IsLoaded(modelData_.material.textureFilepath);
        textureHandle_ = TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilepath);
        if (isNew) {
            common_->WaitAndResetCommandList();
            TextureManager::GetInstance()->ReleaseIntermediateResources();
        }
    }
}


void BaseObject::CreateModel(ModelBuilder::ModelType type, const std::string& textureFilepath) {
    modelData_ = ModelBuilder::CreateModel(type);

    // 基底クラスのメソッドを利用してリソース作成
    CreateVertexResource(modelData_.vertices);
    CreateIndexResource(modelData_.indices);
    CreateMaterialResource();

    // Object3D特有のリソース作成
    SetupResources();

    if (!textureFilepath.empty()) {
        bool isNew = !TextureManager::GetInstance()->IsLoaded(textureFilepath);
        textureHandle_ = TextureManager::GetInstance()->LoadTexture(textureFilepath);
        if (isNew) {
            common_->WaitAndResetCommandList();
            TextureManager::GetInstance()->ReleaseIntermediateResources();
        }
    }
}

void BaseObject::CopyModelResourceFrom(const BaseObject& other, bool shareMaterial) {
    primitiveType_ = other.primitiveType_;
    modelData_ = other.modelData_;
    vertexResource_ = other.vertexResource_;
    vertexData_ = other.vertexData_;
    vertexBufferView_ = other.vertexBufferView_;
    indexResource_ = other.indexResource_;
    indexBufferView_ = other.indexBufferView_;
    textureHandle_ = other.textureHandle_;
    envTextureHandle_ = other.envTextureHandle_;
    isResourceOwner_ = false;

    if (shareMaterial) {
        materialResource_ = other.materialResource_;
        materialData_ = other.materialData_;
        isMaterialOwner_ = false;
    } else {
        CreateMaterialResource();
    }

    // 固有リソース（WVP・カメラバッファ）のみ生成
    SetupResources();
}


void BaseObject::CreateVertexResource(const std::vector<VertexData>& vertices) {
    // 頂点バッファの生成
    vertexResource_ = CreateBufferResource(device_, sizeof(VertexData) * vertices.size());

    // ビューの設定
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // データのマッピングとコピー、即時アンマップ（初期化後はCPUから書き換えない）
    void* mappedData = nullptr;
    vertexResource_->Map(0, nullptr, &mappedData);
    std::memcpy(mappedData, vertices.data(), sizeof(VertexData) * vertices.size());
    vertexResource_->Unmap(0, nullptr);
    vertexData_ = nullptr;
    isResourceOwner_ = true;
}

void BaseObject::CreateIndexResource(const std::vector<uint32_t>& indices) {
    if (indices.empty()) {
        return;
    }

    // インデックスバッファの生成
    indexResource_ = CreateBufferResource(device_, sizeof(uint32_t) * indices.size());

    // ビューの設定
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indices.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // データのマッピングとコピー、即時アンマップ
    void* indexData = nullptr;
    indexResource_->Map(0, nullptr, &indexData);
    std::memcpy(indexData, indices.data(), sizeof(uint32_t) * indices.size());
    indexResource_->Unmap(0, nullptr);
}

void BaseObject::CreateMaterialResource() {
    // マテリアルバッファの生成
    materialResource_ = CreateBufferResource(device_, sizeof(Material));

    // データのマッピングと初期値設定
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData_->enableLighting = true;
    materialData_->uvTransform = MakeIdentity4x4();
    materialData_->enableEnvironmentMap = 1;       // デフォルトON
    materialData_->environmentCoefficient = 0.5f;
    isMaterialOwner_ = true;
}

void BaseObject::DrawImGui(const std::string& label) {
#ifdef USE_IMGUI
    if (!materialData_) return;

    // オブジェクトごとに一意の折りたたみノードを作成
    if (ImGui::TreeNode(label.c_str())) {

        if (ImGui::TreeNode("Base Material")) {
            // カラー編集
            ImGui::ColorEdit4("Color", &materialData_->color.x);

            // ライティングON/OFF (int32_t を bool に安全にキャスト変換して操作)
            bool enableLight = (materialData_->enableLighting != 0);
            if (ImGui::Checkbox("Enable Lighting", &enableLight)) {
                materialData_->enableLighting = enableLight ? 1 : 0;
            }

            ImGui::Separator();

            // 環境マッピングON/OFF
            bool enableEnv = (materialData_->enableEnvironmentMap != 0);
            if (ImGui::Checkbox("Enable EnvMap", &enableEnv)) {
                materialData_->enableEnvironmentMap = enableEnv ? 1 : 0;
            }

            // 環境マッピングの影響度 (0.0 ～ 1.0)
            if (enableEnv) {
                ImGui::SliderFloat("EnvMap Coefficient", &materialData_->environmentCoefficient, 0.0f, 1.0f);
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
#endif
}