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

void ParticleManager::Emit(const std::string& name, Vector3 position, Vector3 range, float duration, float minLifetime, float maxLifetime) {
    if (particleGroups_.contains(name)) {
        particleGroups_[name]->Emit(position, range, duration, minLifetime, maxLifetime);
        particleGroups_[name]->Begin();
    }
}