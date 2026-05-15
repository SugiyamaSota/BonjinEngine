#include "Particle.h"

#include<numbers>

#include "TextureManager.h"


Particle::Particle() :BaseObject() {

	// 乱数エンジンの初期化
	std::random_device seed_gen;
	randomEngine_ = std::mt19937(seed_gen());

	for (uint32_t index = 0; index < kNumInstance_; ++index)
	{
		particles_[index].transform = InitializeWorldTransform();
		particles_[index].lifeTime = 0.0f;
		particles_[index].currentTime = 0.0f;
	}
}

Particle::~Particle() {
	// 1. 基底クラスの共通リソース（頂点・マテリアル）を解放
	BaseObject::ReleaseResources();

	// 2. Particle 固有のリソースを解放
	if (instancingResource_) {
		instancingResource_->Unmap(0, nullptr);
		instancingResource_.Reset();
	}
	instancingData_ = nullptr;
}

void Particle::Update(Camera* camera) {
	const float kDeltaTime = 1.0f / 60.0f;

	Matrix4x4 backTofrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
	Matrix4x4 billboardMatrix = Multiply(backTofrontMatrix, Inverse(camera->GetViewMatrix()));
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	for (uint32_t index = 0; index < kNumInstance_; ++index) {
		// 生存していない（lifeTimeが0）パーティクルはスキップ
		if (particles_[index].lifeTime <= 0.0f) continue;

		// 時間を進める
		particles_[index].currentTime += kDeltaTime;


		// 寿命判定
		if (particles_[index].currentTime >= particles_[index].lifeTime) {
			particles_[index].lifeTime = 0.0f;
			// 描画されないようにスケールを0にする
			instancingData_[index].WVP = MakeScaleMatrix({ 0.0f, 0.0f, 0.0f });
			continue;
		}

		// 更新処理
		if (particles_[index].updateFunc) {
			particles_[index].updateFunc(particles_[index], kDeltaTime);
		} else {
			particles_[index].transform.translate = Add(particles_[index].transform.translate, particles_[index].velocity);
		}
		// 2. アルファ値の計算（1.0 -> 0.0）
		float elapsedRatio = particles_[index].currentTime / particles_[index].lifeTime;
		particles_[index].color.w = 1.0f - elapsedRatio;

		Matrix4x4 localRotationMatrix = MakeRotateMatrix(particles_[index].transform.rotate);

		// ビルボード（カメラ向き）と自身の回転を合成
		// ※順番は 自身の回転 -> ビルボード向き の順が一般的です
		Matrix4x4 combinedRotation = Multiply(localRotationMatrix, billboardMatrix);

		Matrix4x4 worldMatrix = MakeAffineMatrix(
			particles_[index].transform.scale,
			combinedRotation, // 合成した回転を適用
			particles_[index].transform.translate
		);

		// 4. WVP行列の計算と定数バッファへの書き込み
		Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
		instancingData_[index].WVP = worldViewProjectionMatrix;
		instancingData_[index].World = worldMatrix;
		instancingData_[index].color = particles_[index].color;
	}
}

void Particle::Draw() {
	// PSOManagerの新しいGetPipelineState関数を呼び出し
	ID3D12PipelineState* pso =
		common_->GetPSO()->GetPipelineState(
			device_,
			PrimitiveType::kParticle,
			BlendMode::kAdd,
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK
		);

	// PSOの設定
	common_->GetCommandList()->SetGraphicsRootSignature(common_->GetPSO()->GetRootSignature(PrimitiveType::kParticle));
	common_->GetCommandList()->SetPipelineState(pso);

	//　モデルの描画
	// VBV
	common_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	// 形状を設定
	common_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// マテリアルCBufferの場所を設定
	common_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	common_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvhandleGPU_);

	// SRV用のdescriptionTavleの先頭を設定
	common_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));
	// 光
	common_->GetCommandList()->SetGraphicsRootConstantBufferView(3, LightManager::GetInstance()->GetDirectionalLightResource()->GetGPUVirtualAddress());
	// 描画
	common_->GetCommandList()->DrawInstanced(UINT(modelData_.vertices.size()), kNumInstance_, 0, 0);
}

void Particle::Emit(const Vector3& position, const Vector3& velocity, const Vector4& color, float lifetime, ParticleUpdateFunc func) {

	std::uniform_real_distribution<float> distRot(0.0f, std::numbers::pi_v<float> *2.0f);

	for (uint32_t index = 0; index < kNumInstance_; ++index) {
		// 寿命が0（非アクティブ）な枠を探す
		if (particles_[index].lifeTime <= 0.0f) {
			// パラメータをセット
			particles_[index].transform.translate = position;
			particles_[index].transform.rotate.z = distRot(randomEngine_);
			particles_[index].velocity = velocity;
			particles_[index].color = color;
			particles_[index].lifeTime = lifetime;
			particles_[index].currentTime = 0.0f;
			particles_[index].transform.scale = { 0.05f, 2.f, 0.05f }; // 基本サイズ
			//particles_[index].transform.scale = { 0.01f, 0.01f, 0.01f }; // 基本サイズ
			particles_[index].updateFunc = func;

			return; // 1つ生成したら終了
		}
	}
}

void Particle::DrawImGui()
{



}

void Particle::SetupResources() {
	// インスタンシング用リソースの生成
	instancingResource_ = CreateBufferResource(device_, sizeof(ParticleForGPU) * kNumInstance_);
	instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));

	for (uint32_t index = 0; index < kNumInstance_; ++index) {
		instancingData_[index].WVP = MakeIdentity4x4();
		instancingData_[index].World = MakeIdentity4x4();
	}

	// SRVの作成
	srvIndex_ = SrvManager::GetInstance()->Allocate();
	SrvManager::GetInstance()->CreateSrv(
		srvIndex_,
		instancingResource_.Get(),
		SrvType::StructuredBuffer,
		kNumInstance_,
		sizeof(ParticleForGPU)
	);
	srvhandleGPU_ = SrvManager::GetInstance()->GetGPUHandle(srvIndex_);
}