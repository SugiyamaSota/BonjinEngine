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

// ParticleManager.cpp に実装を追加
void ParticleManager::CreateParticleGroup(const std::string& name, ModelBuilder::ModelType type, const std::string& textureFilepath) {
	if (particleGroups_.contains(name)) return;

	auto newGroup = std::make_unique<Particle>();
	// Particleクラスに追加したCreateModelを呼び出す
	newGroup->CreateModel(type, textureFilepath);

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

void ParticleManager::Emit(const std::string& name, const ParticleConfig& config) {
	if (particleGroups_.contains(name)) {
		particleGroups_[name]->Emit(config);
	}
}

std::vector<std::string> ParticleManager::GetGroupNames() const {
	std::vector<std::string> names;
	names.reserve(particleGroups_.size());
	for (const auto& [name, group] : particleGroups_) {
		names.push_back(name);
	}
	return names;
}

uint32_t ParticleManager::GetActiveCount(const std::string& name) const {
	auto it = particleGroups_.find(name);
	return (it != particleGroups_.end() && it->second) ? it->second->GetActiveCount() : 0;
}

uint32_t ParticleManager::GetMaxCountPerGroup() const {
	return Particle::GetMaxCount();
}

void ParticleManager::Clear(const std::string& name) {
	if (particleGroups_.contains(name)) {
		particleGroups_[name]->Clear();
	}
}

void ParticleManager::SetUVTransform(const std::string& name, const Matrix4x4& transform) {
	if (particleGroups_.contains(name)) {
		particleGroups_[name]->SetUVTransform(transform);
	}
}

void ParticleManager::SetUseBillboard(const std::string& name, bool useBillboard) {
	auto it = particleGroups_.find(name);
	if (it != particleGroups_.end() && it->second) {
		it->second->SetUseBillboard(useBillboard);
	}
}

void ParticleManager::SetCullMode(const std::string& name, CullMode cullMode) {
	auto it = particleGroups_.find(name);
	if (it != particleGroups_.end() && it->second) {
		it->second->BaseObject::SetCullMode(cullMode);
	}
}
