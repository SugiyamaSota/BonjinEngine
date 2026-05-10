#include "ParticleManager.h"

#include "Particle.h"

ParticleManager* ParticleManager::GetInstance() {
	static ParticleManager instance;
	return &instance;
}

void ParticleManager::Finalize() {
	particleGroups_.clear();
}

void ParticleManager::Initialize() {
	srvManager_ = SrvManager::GetInstance();
	dxCommon_ = DirectXCommon::GetInstance();
}

void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& modelFileName) {
	if (particleGroups_.contains(name)) return;

	auto newGroup = std::make_unique<Particle>();
	newGroup->LoadModel(modelFileName); // ここで内部的にSRV確保を行うよう修正
	particleGroups_[name] = std::move(newGroup);
}

void ParticleManager::Update(Camera* camera) {
	// 全てのパーティクルグループについて処理
	for (auto& [name, group] : particleGroups_) {
		group->Update(camera); // 内部の二重for文（パーティクル個々の更新）はParticleクラス側で実行
	}
}

void ParticleManager::Draw() {
	// スライドの「描画」手順：共通設定を先に行う
	auto commandList = dxCommon_->GetCommandList();



	for (auto& [name, group] : particleGroups_) {
		group->Draw(); // 1グループ1DrawCall
	}
}

void ParticleManager::Emit(const std::string& name, const Vector3& position, uint32_t count) {

	static std::random_device seed_gen;
	static std::mt19937 engine(seed_gen());

	// -0.5f ～ 0.5f の間でランダムな数値を出す
	std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

	if (particleGroups_.contains(name)) {
		// 例: 1回のEmitで複数のパーティクルをランダムな方向に飛ばすロジック
		for (uint32_t i = 0; i < count; ++i) {
			Vector3 velocity = {
			0,0,0
			};
			Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			float lifetime = 1.0f;

			auto gravityBehavior = [](ParticleData& p, float dt) {
				//p.velocity.y -= 0.01f; // 重力を加える
				//p.transform.translate = Add(p.transform.translate, p.velocity);
				};

			// Particleクラス側の「新しいEmit」を呼ぶ
			particleGroups_[name]->Emit(position, velocity, { 1,1,1,1 }, 1.0f, gravityBehavior);
		}
	}
}