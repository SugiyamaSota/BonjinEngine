#pragma once
#include"ModelBuilder.h"

class Model {
public:
	/// --- 汎用関数 ---
	/// <summary>
	///  コンストラクタ
	/// </summary>
	Model();

	~Model();

	/// <summary>
	/// モデルをロード
	/// </summary>
	/// <param name="fileName">モデル名</param>
	void LoadModel(const std::string& derectoryName, const std::string& fileName);

	/// <summary>
	/// 球体モデルを作成
	/// </summary>
	/// <param name="subdivision">分割数</param>
	void CreateSphere(uint32_t subdivision);

	void CreateCube();

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="worldTransform">ワールドトランスフォーム</param>
	/// <param name="camera">カメラ</param>
	void Update(WorldTransform worldTransform, Camera* camera);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	void DrawImGui();

	/// --- 設定関数 ---
	/// <summary>
	/// ライティングの有無
	/// </summary>
	/// <param name="enableLighting">フラグ</param>
	void SetEnableLighting(bool enableLighting) { materialData_->enableLighting = enableLighting; }

	/// <summary>
	/// 色と透明度
	/// </summary>
	/// <param name="color">設定したい色と透明度</param>
	void SetColor(Vector4 color) { materialData_->color = color; }


	void SetFillMode(D3D12_FILL_MODE fillMode) { fillMode_ = fillMode; }
	void SetCullMode(D3D12_CULL_MODE cullMode) { cullMode_ = cullMode; }
	void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }

	/// --- 取得関数 ---
	float GetAlpha() { return materialData_->color.w; }


private:
	/// --- 変数 ---
	// モデルデータ
	const ModelData* modelData_ = nullptr;

	// 頂点リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	VertexData* vertexData_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	// マテリアルリソース
	Material* materialData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;

	// WVPリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpData_ = nullptr;

	// カメラresource
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_ = nullptr;
	CameraForGPU* cameraData_ = nullptr;

	// トランスフォーム
	WorldTransform transform_;

	// ビュー行列
	Matrix4x4 viewMatrix_;
	// 射影行列
	Matrix4x4 projectionMatrix_;
	// ビュープロジェクション行列
	Matrix4x4 viewProjectionMatrix_;

	// テクスチャハンドル
	int textureHandle_ = 0;

	// dxcommonのインスタンス
	DirectXCommon* common = nullptr;

	// 各モードの変数
	D3D12_FILL_MODE fillMode_ = D3D12_FILL_MODE_SOLID; // デフォルト設定
	D3D12_CULL_MODE cullMode_ = D3D12_CULL_MODE_BACK;   // デフォルト設定
	BlendMode blendMode_ = BlendMode::kNone;     // デフォルト設定

	//
	void SetupResources();
};