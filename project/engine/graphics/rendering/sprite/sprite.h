#pragma once
#include"../bonjin/BonjinEngine.h"
#include"../color/Color.h"

namespace Bonjin {

	class Sprite {
	public:
		Sprite();
		~Sprite(); // リソースを解放するためのデストラクタ

		/// <summary>
		/// スプライトの初期化
		/// </summary>
		/// <param name="worldTransform">ワールド変換</param>
		/// <param name="textureFilePath">テクスチャファイルのパス</param>
		void Initialize(const std::string& textureFilePath);

		/// <summary>
		/// スプライトの更新
		/// </summary>
		/// <param name="worldTransform">ワールド変換</param>
		/// <param name="color">色</param>
		void Update();

		/// <summary>
		/// スプライトの描画
		/// </summary>
		void Draw();

		// ImGuiの描画
		void DrawImGui();

		// 各項目のゲッターセッター
		Vector3& Anchor() { return anchor_; }
		Vector2& Size() { return size_; }
		Vector2& Scale() { return scale_; }
		Vector2& Rotate() { return rotate_; }
		Vector2& Translate() { return translate_; }
		Vector4&  Color() { return color_; }

		void SetFlipX(bool flag) { isFlipX_ = flag; }
		void SetFlipY(bool flag) { isFlipY_ = flag; }

	private:
		// 頂点リソース
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
		VertexData* vertexData_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

		// インデックスリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
		uint32_t* indexData_ = nullptr;
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

		// マテリアルリソース
		Material* materialData_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;

		// WVP リソース
		TransformationMatrix* wvpData_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;

		// 各項目
		Vector3 anchor_;
		Vector2 size_;
		Vector2 scale_;
		Vector2 rotate_;
		Vector2 translate_;
		Vector4 color_;

		int textureHandle_ = 0;

		// dxCommon変数
		DirectXCommon* dxCommon_;

		// デバイス
		ID3D12Device* device_;

		// スプライト用pso
		ID3D12PipelineState* pso_;

		// ビュー行列
		Matrix4x4 viewMatrix_;
		// 射影行列
		Matrix4x4 projectionMatrix_;
		// ビュープロジェクション行列
		Matrix4x4 viewProjectionMatrix_;

		bool isFlipX_ = false;
		bool isFlipY_ = false;

		void UpdateUVTransform();

		void CorrectionVertexData();
	};

}