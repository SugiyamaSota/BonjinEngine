#pragma once
#include "ModelBuilder.h"
#include <wrl/client.h>
#include <string>

#include "RenderTypes.h"

namespace Bonjin {
	class IScene;
}

class BaseObject {
public:
	virtual ~BaseObject();

	void LoadModel(const std::string& directoryName, const std::string& fileName);
	void SetModel(const std::string& modelKey);
	void CreateModel(ModelBuilder::ModelType type, const std::string& textureFilepath);
	void CopyModelResourceFrom(const BaseObject& other, bool shareMaterial = true);

	// 共通の描画インターフェース
	virtual void Draw() = 0;

	virtual void DrawImGui(const std::string& label);

	// オブジェクト名
	const std::string& GetName() const { return name_; }
	void SetName(const std::string& name) { name_ = name; }
	void SetParentScene(Bonjin::IScene* parent) { parentScene_ = parent; }

	// 共通のセッター
	void SetFillMode(FillMode fillMode) { fillMode_ = fillMode; }
	void SetCullMode(CullMode cullMode) { cullMode_ = cullMode; }
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
	bool isMaterialOwner_ = true;
	bool isResourceOwner_ = true;

	int textureHandle_ = 0;
	int envTextureHandle_ = 0;

	// 各モードの変数
	FillMode fillMode_ = FillMode::kSolid;
	CullMode cullMode_ = CullMode::kBack;
	BlendMode blendMode_ = BlendMode::kNone;
	PrimitiveType primitiveType_ = PrimitiveType::kObject3D;

	std::string name_ = "Object3D";
	Bonjin::IScene* parentScene_ = nullptr;
};
