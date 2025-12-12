#include "Sprite.h"
#include <cassert>

namespace Bonjin {

	Sprite::Sprite()
	{
		viewMatrix_ = MakeIdentity4x4();
		projectionMatrix_ = MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(1280), static_cast<float>(720), 0.0f, 100.0f);
		viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

		// 初期値で各項目を初期化
		anchor_ = { 0.f,0.f };       // 中心が基準
		size_ = { 0.f,0.f };         // サイズは0(描画されない)
		scale_ = { 1.f,1.f };        // 等倍で描画
		rotate_ = { 0.f,0.f };       // 回転無し
		translate_ = { 0.f,0.f };    // 原点
		color_ = { 1.f,1.f,1.f,1.f };// 白

	}

	Sprite::~Sprite()
	{
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

	void Sprite::Initialize(const std::string& textureFilePath) {

		// スプライトの幅と高さを定義
		float width = size_.x;  // 現在の頂点データに基づく幅
		float height = size_.y; // 現在の頂点データに基づく高さ

		// 頂点バッファの生成
		vertexResource_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), sizeof(VertexData) * 4);
		vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
		vertexBufferView_.StrideInBytes = sizeof(VertexData);
		vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

		// 頂点データの定義 (正方形)
		// anchorの概念を導入し、中心を原点とするように調整
		// 例: anchor = {0.5f, 0.5f, 0.0f} で中心を意味する
		float left = -width * anchor_.x;
		float right = width * (1.0f - anchor_.x);
		float top = -height * anchor_.y;
		float bottom = height * (1.0f - anchor_.y);

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
		materialData_->color = color_;
		materialData_->enableLighting = false; // スプライトはライティングを無効にする
		materialData_->uvTransform = MakeIdentity4x4(); // UV変換を初期化

		// WVP リソースの生成
		wvpResource_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
		wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
		wvpData_->WVP = MakeIdentity4x4(); // WVP 行列を初期化
		wvpData_->World = MakeIdentity4x4(); // ワールド行列を初期化

		// テクスチャのロード
		textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/textures/" + textureFilePath);
	}

	void Sprite::Update() {
		CorrectionVertexData();

		WorldTransform transform = InitializeWorldTransform();

		transform.scale = { scale_.x, scale_.y,0.f };
		transform.rotate = { rotate_.x,rotate_.y,0.f };
		transform.translate = { translate_.x,translate_.y,0.f };

		// ワールド行列の計算
		wvpData_->World = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

		// WVP 行列の計算
		Matrix4x4 worldViewProjectionMatrix = Multiply(wvpData_->World, viewProjectionMatrix_);
		wvpData_->WVP = worldViewProjectionMatrix;

		// マテリアルの更新
		materialData_->color = color_;

		UpdateUVTransform();
	}

	void Sprite::Draw() {

		DirectXCommon* common = DirectXCommon::GetInstance();

		// インデックスバッファビューの設定
		common->GetCommandList()->IASetIndexBuffer(&indexBufferView_);
		// 頂点バッファビューの設定
		common->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);


		// マテリアルCBufferの場所を設定
		common->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
		// WVP CBufferの場所を設定
		common->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
		// テクスチャのDescriptorTableを設定
		common->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

		// 描画コマンド
		common->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0); // 6つのインデックス (2つの三角形) を描画
	}

	void Sprite::UpdateUVTransform() {
		// スケーリング行列を計算 (反転の有無に応じて -1.0f または 1.0f を設定)
		Vector3 scale = {
			isFlipX_ ? -1.0f : 1.0f,
			isFlipY_ ? -1.0f : 1.0f,
			1.0f
		};

		// 1. 変換の中心 (0.5, 0.5) へ平行移動
		Matrix4x4 translateToCenter = MakeTranslateMatrix({ 0.5f, 0.5f, 0.0f });
		// 2. スケーリング (反転)
		Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
		// 3. 元の場所 (-0.5, -0.5) へ戻す平行移動
		Matrix4x4 translateBack = MakeTranslateMatrix({ -0.5f, -0.5f, 0.0f });

		// 1. 基準点を中心 (0.5, 0.5) に持ってくる (逆移動)
		Matrix4x4 matrix = Multiply(scaleMatrix, translateBack);
		// 2. 反転 (スケーリング)
		matrix = Multiply(matrix, translateToCenter);

		// 単純な方法: スケーリングと移動を組み合わせる
		materialData_->uvTransform = MakeAffineMatrix(
			scale,
			Vector3{ 0.0f, 0.0f, 0.0f }, // 回転はなし
		{
			isFlipX_ ? 1.0f : 0.0f,
			isFlipY_ ? 1.0f : 0.0f,
			0.0f
		}
		);
	}

	void Sprite::DrawImGui() {

		ImGui::Separator();
		ImGui::Text("sprite");
		ImGui::Checkbox("flipX", &isFlipX_);
		ImGui::Checkbox("flipY", &isFlipY_);

	}

	void Sprite::CorrectionVertexData() {
		// スプライトの幅と高さを定義
		float width = size_.x;  // 現在の頂点データに基づく幅
		float height = size_.y; // 現在の頂点データに基づく高さ

		// 頂点バッファの生成
		vertexResource_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), sizeof(VertexData) * 4);
		vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
		vertexBufferView_.StrideInBytes = sizeof(VertexData);
		vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

		// 頂点データの定義 (正方形)
		// anchorの概念を導入し、中心を原点とするように調整
		// 例: anchor = {0.5f, 0.5f, 0.0f} で中心を意味する
		float left = -width * anchor_.x;
		float right = width * (1.0f - anchor_.x);
		float top = -height * anchor_.y;
		float bottom = height * (1.0f - anchor_.y);

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
	}

}