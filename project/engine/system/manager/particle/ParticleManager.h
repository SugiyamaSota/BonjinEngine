#pragma once
#include <map>
#include <string>
#include <memory>
#include <vector>
#include "../camera/Camera.h"
#include "../srv/SrvManager.h"
#include "Particle.h"

class ParticleManager {
public:
    static ParticleManager* GetInstance();

    // 初期化：DirectXCommonとSRVマネージャの参照を保持
    void Initialize();

    // パーティクルグループの作成
    void CreateParticleGroup(const std::string& name, ModelBuilder::ModelType type, const std::string& textureFilepath);

    // 更新処理：全グループをループ処理
    void Update(Camera* camera);

    // 描画処理：全グループをループ処理
    void Draw();

    void Finalize();

    // エミット（発生）
    void Emit(const std::string& name, const ParticleConfig& config);
    void Clear(const std::string& name);
    void SetUVTransform(const std::string& name, const Matrix4x4& transform);

    // --- デバッグ用 ---
    // 登録されているグループ名の一覧
    std::vector<std::string> GetGroupNames() const;
    // グループの生存中パーティクル数 (グループが無ければ0)
    uint32_t GetActiveCount(const std::string& name) const;
    // 1グループあたりの最大数
    uint32_t GetMaxCountPerGroup() const;

private:
    ParticleManager() = default;
    ~ParticleManager() = default;

    // コピーコンストラクタと代入演算子を禁止する
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    // グループ名をキーにしてパーティクルを管理
    std::map<std::string, std::unique_ptr<Particle>> particleGroups_;
};
