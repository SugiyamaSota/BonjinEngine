#include "Particle.h"

#include<numbers>

Particle::Particle() {
	common = DirectXCommon::GetInstance();

	// 乱数エンジンの初期化
	std::random_device seed_gen;
	randomEngine_ = std::mt19937(seed_gen());

	for (uint32_t index = 0; index < kNumInstance_; ++index)
	{
		particles_[index].transform = InitializeWorldTransform();
		particles_[index].lifeTime = 0.0f;
		particles_[index].currentTime = 0.0f;
	}
	viewMatrix_ = MakeIdentity4x4();
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(1280) / float(720), 0.1f, 100.0f);
	viewProjectionMatrix_ = MakeIdentity4x4();
	activeNum_ = 0;
}

void Particle::LoadModel(const std::string& fileName) {
	// モデルファイル読み込み
	modelData_ = ModelBuilder::LoadObjFile("resources/models/" + fileName, fileName + ".obj");

	// 頂点用のリソース
	vertexResource_ = CreateBufferResource(common->GetDevice(), sizeof(VertexData) * modelData_.vertices.size());
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	std::memcpy(vertexData_, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());

	// マテリアル用のリソース
	materialResource_ = CreateBufferResource(common->GetDevice(), sizeof(Material));
	materialData_ = nullptr;
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->enableLighting = false;
	materialData_->uvTransform = MakeIdentity4x4();

	// WVP用のリソース
	instancingResource_ = CreateBufferResource(common->GetDevice(), sizeof(ParticleForGPU) * kNumInstance_);
	instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));

	for (uint32_t index = 0; index < kNumInstance_; ++index)
	{
		instancingData_[index].WVP = MakeIdentity4x4();
		instancingData_[index].World = MakeIdentity4x4();
	}

	// テクスチャ
	textureHandle_ = TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilepath);
	common->WaitAndResetCommandList();
	TextureManager::GetInstance()->ReleaseIntermediateResources();


	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = kNumInstance_;
	srvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
	srvhandleCPU_ = GetCPUDescriptorHandle(DirectXCommon::GetInstance()->GetSRVDescriptorHeap(), DirectXCommon::GetInstance()->GetSRVSize(), 3);
	srvhandleGPU_ = GetGPUDescriptorHandle(DirectXCommon::GetInstance()->GetSRVDescriptorHeap(), DirectXCommon::GetInstance()->GetSRVSize(), 3);
	common->GetDevice()->CreateShaderResourceView(instancingResource_.Get(), &srvDesc, srvhandleCPU_);
}

void Particle::Update(Camera* camera) {
	// フレームごとの経過時間 (deltaTime)
	const float kDeltaTime = 1.0f / 60.0f; // 実際のフレームレートに合わせて変更してください

	// 1. システム全体のタイマー更新
	if (isSystemActive_) {
		durationTimer_ -= kDeltaTime;
		if (durationTimer_ < 0.0f) {
			durationTimer_ = 0.0f;
			isSystemActive_ = false;
		}
	}

	// 2. 個々のパーティクルの更新
	for (uint32_t index = 0; index < kNumInstance_; ++index)
	{
		// パーティクルが「有効」な場合のみ時間を進める (lifeTime > 0で有効とみなす)
		if (particles_[index].lifeTime > 0.0f) {
			particles_[index].currentTime += kDeltaTime;
		}

		// 生存時間を超えたかどうかのチェック
		if (particles_[index].currentTime >= particles_[index].lifeTime) {

			if (isSystemActive_) {
				// システム稼働中: パーティクルを再初期化（即座に再始動/ループ）
				InitializeParticle(index);

			} else {
				// システム終了済み: パーティクルを「死亡」状態にし、描画されないようにする
				particles_[index].lifeTime = 0.0f;
			}
		}

		// パーティクルが有効（lifeTime > 0）の場合
		if (particles_[index].lifeTime > 0.0f)
		{
			//
			Matrix4x4 backTofrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);;

			Matrix4x4 billboardMatrix = Multiply(backTofrontMatrix, Inverse(camera->GetViewMatrix()));
			billboardMatrix.m[3][0] = 0.f;
			billboardMatrix.m[3][1] = 0.f;
			billboardMatrix.m[3][2] = 0.f;

			particles_[index].transform.translate = Add(particles_[index].transform.translate, particles_[index].velocity);

			// 経過時間 (0.0f:開始 -> 1.0f:終了)
			float elapsedRatio = particles_[index].currentTime / particles_[index].lifeTime;

			// 透明度 (Alpha) の計算: 1.0f (不透明) -> 0.0f (透明)
			float alpha = 1.0f - elapsedRatio;

			// 最終的な色を計算し、アルファ値を適用
			particles_[index].color.w = alpha;
			// 寿命が尽きるタイミングで透明度が 0.0f になるように適用

			// ワールド行列の計算
			Matrix4x4 worldMatrix = MakeAffineMatrix(particles_[index].transform.scale, billboardMatrix, particles_[index].transform.translate);

			// WVP行列の計算と設定
			Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
			instancingData_[index].WVP = worldViewProjectionMatrix;
			instancingData_[index].World = worldMatrix;
			instancingData_[index].color = particles_[index].color;

		} else {
			// システム終了後、死亡したパーティクルは見えなくする
			// WVP行列をスケール0の行列にすることで、描画パイプラインから隠す
			instancingData_[index].WVP = MakeScaleMatrix({ 0.0f, 0.0f, 0.0f });
			instancingData_[index].World = MakeIdentity4x4();
			instancingData_[index].color = particles_[index].color;
		}
	}
}

void Particle::Draw() {
	// PSOを遅延生成/取得するためにデバイスが必要
	ID3D12Device* device = common->GetDevice();

	// PSOManagerの新しいGetPipelineState関数を呼び出し
	ID3D12PipelineState* pso =
		common->GetPSO()->GetPipelineState(
			device,
			PrimitiveType::kParticle,
			BlendMode::kNormal,
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK
		);

	// PSOの設定
	common->GetCommandList()->SetGraphicsRootSignature(common->GetPSO()->GetRootSignature(PrimitiveType::kParticle));
	common->GetCommandList()->SetPipelineState(pso);

	//　モデルの描画
	// VBV
	common->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	// 形状を設定
	common->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// マテリアルCBufferの場所を設定
	common->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	common->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvhandleGPU_);

	// SRV用のdescriptionTavleの先頭を設定
	common->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));
	// 光
	common->GetCommandList()->SetGraphicsRootConstantBufferView(3, common->GetDirectionalLightResource()->GetGPUVirtualAddress());
	// 描画
	common->GetCommandList()->DrawInstanced(UINT(modelData_.vertices.size()), kNumInstance_, 0, 0);
}

void Particle::Emit(Vector3 position, Vector3 range, float duration, float minLifetime, float maxLifetime)
{

	// システムの全体設定を保存
	durationTimer_ = duration;
	minLifetime_ = minLifetime;
	maxLifetime_ = maxLifetime;
	emitPosition_ = position;
	emitRange_ = range;

	// 既存の設定をデバッグ用変数に格納
	minLifetimeDebug_ = minLifetime_;
	maxLifetimeDebug_ = maxLifetime_;
	emitPositionDebug_ = emitPosition_;
	emitRangeDebug_ = emitRange_;

}

void Particle::InitializeParticle(uint32_t index)
{

	/// --- 乱数分布の定義 ---
	std::uniform_real_distribution<float> distX(-emitRange_.x, emitRange_.x);
	std::uniform_real_distribution<float> distY(-emitRange_.y, emitRange_.y);
	std::uniform_real_distribution<float> distZ(-emitRange_.z, emitRange_.z);
	std::uniform_real_distribution<float> distVel(-0.1f, 0.1f);
	std::uniform_real_distribution<float> distColor(0.f, 1.f);
	std::uniform_real_distribution<float> distLifetime(minLifetime_, maxLifetime_);

	/// --- 既存のデータをリセット ---
	particles_[index].transform = InitializeWorldTransform(); // 座標
	particles_[index].lifeTime = 0.f; // 生存時間
	particles_[index].currentTime = 0.f; // 経過時間
	particles_[index].velocity = {}; // 速度
	particles_[index].color = {}; // 色

	/// --- 各データを各々セット ---
	// ランダムなオフセットを生成して中心座標に加算
	Vector3 randomOffset = {
		distX(randomEngine_),
		distY(randomEngine_),
		distZ(randomEngine_)
	};
	particles_[index].transform.translate = Add(emitPosition_, randomOffset);

	// 生存時間を乱数で設定
	particles_[index].lifeTime = distLifetime(randomEngine_);

	// 速度を乱数で設定
	Vector3 randomVelocity = {
	distVel(randomEngine_),
	distVel(randomEngine_),
	distVel(randomEngine_)
	};
	particles_[index].velocity = randomVelocity;

	// 色をアルファ値以外乱数で設定
	particles_[index].color = {
		distColor(randomEngine_),
		distColor(randomEngine_),
		distColor(randomEngine_),
		1.f
	};

}

void Particle::Begin() 
{

	// 起動済みならスルー
	if (isSystemActive_) 
	{
		return;
	}

	// フラグをtrueに変更
	isSystemActive_ = true;

	// 全てのインスタンスを初期化
	for (uint32_t index = 0; index < kNumInstance_; ++index) 
	{
		InitializeParticle(index);
	}

}

void Particle::End() {

	// 起動してないならスルー
	if (!isSystemActive_) {
		return;
	}

	// フラグをfalseに変更
	isSystemActive_ = false;

}

void Particle::DrawImGui()
{

	// 横線
	ImGui::Separator();

	// オブジェクト名
	ImGui::Text("Particle");

	// emitter設定
	if (ImGui::CollapsingHeader("Emit Option"))
	{

		ImGui::InputFloat3("StartPosition", &emitPositionDebug_.x); // emitterの中心座標
		ImGui::InputFloat3("EmitRange", &emitRangeDebug_.x); // emitterの範囲
		ImGui::InputFloat("MinLifeTime", &minLifetimeDebug_); // 最低生存時間
		ImGui::InputFloat("MaxLifeTime", &maxLifetimeDebug_); // 最大生存時間
		if (ImGui::Button("Confirm"))
		{
			Emit(emitPositionDebug_, emitRangeDebug_, durationTimer_, minLifetimeDebug_, maxLifetimeDebug_); // 設定を反映
		}

	}
	
	if (!isSystemActive_) 
	{
		if (ImGui::Button("Begin")) 
		{
			Begin();
		}

	} else {

		if (ImGui::Button("End")) 
		{
			End();
		}

	}

}