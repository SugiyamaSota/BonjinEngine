#pragma once
#include <map>
#include <string>
#include <memory>
#include "Particle.h"
#include "../camera/Camera.h"
#include "../srv/SrvManager.h"

class ParticleManager {
public:
    static ParticleManager* GetInstance();

    void Finalize();

    // 初期化：DirectXCommonとSRVマネージャの参照を保持
    void Initialize();

    // パーティクルグループの作成
    void CreateParticleGroup(const std::string& name, const std::string& modelFileName);

    // 更新処理：全グループをループ処理
    void Update(Camera* camera);

    // 描画処理：全グループをループ処理
    void Draw();

    // エミット（発生）
    void Emit(const std::string& name, Vector3 position, Vector3 range, float duration, float minLifetime, float maxLifetime);

private:
    ParticleManager() = default;
    ~ParticleManager() = default;
    static ParticleManager* instance_;

    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    // グループ名をキーにしてパーティクルを管理
    std::map<std::string, std::unique_ptr<Particle>> particleGroups_;
};