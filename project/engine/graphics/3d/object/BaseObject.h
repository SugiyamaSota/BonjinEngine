#pragma once
#include "ModelBuilder.h"
#include <wrl/client.h>

class BaseObject {
public:
	virtual ~BaseObject() {
		ReleaseResources();
	}

	void LoadModel(const std::string& directoryName, const std::string& fileName);
	void CreateModel(ModelBuilder::ModelType type, const std::string& textureFilepath);

	// 共通の描画インターフェース
	virtual void Draw() = 0;

	virtual void DrawImGui(const std::string& label);

	// 共通のセッター
	void SetFillMode(D3D12_FILL_MODE fillMode) { fillMode_ = fillMode; }
	void SetCullMode(D3D12_CULL_MODE cullMode) { cullMode_ = cullMode; }
	void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
	void SetPrimitiveType(PrimitiveType primitiveType) { primitiveType_ = primitiveType; }
	void SetColor(const Vector4& color) { if (materialData_) materialData_->color = color; }
	void SetEnableLighting(bool enable) { if (materialData_) materialData_->enableLighting = enable; }
	void SetEnableEnableEnvironmentMap(bool flag) { if (materialData_) materialData_->enableEnvironmentMap = flag; }
	void SetUVTransform(const Matrix4x4& transform) { if (materialData_) materialData_->uvTransform = transform; }
	const ModelData& GetModelData() const { return modelData_; }

protected:
	BaseObject();

	// リソース生成の共通化
	void CreateVertexResource(const std::vector<VertexData>& vertices);
	void CreateIndexResource(const std::vector<uint32_t>& indices);
	void CreateMaterialResource();
	virtual void SetupResources() = 0;
	void ReleaseResources();

	// 共通メンバ
	DirectXCommon* common_ = nullptr;
	ID3D12Device* device_ = nullptr;

	// モデル・リソース
	ModelData modelData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	VertexData* vertexData_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	int textureHandle_ = 0;
	int envTextureHandle_ = 0;

	// 各モードの変数
	D3D12_FILL_MODE fillMode_ = D3D12_FILL_MODE_SOLID;
	D3D12_CULL_MODE cullMode_ = D3D12_CULL_MODE_BACK;
	BlendMode blendMode_ = BlendMode::kNone;
	PrimitiveType primitiveType_ = PrimitiveType::kObject3D;
};
