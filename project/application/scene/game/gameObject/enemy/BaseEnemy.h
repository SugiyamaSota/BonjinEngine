#pragma once
#include "../../logic/Data.h"
#include "../GameObject.h"
#include "EnemyBullet.h"
#include "Object3D.h"
#include <list>
#include <memory>

class Player;

namespace Bonjin {

class BaseEnemy : public GameObject {
public:
    virtual ~BaseEnemy() = default;

    virtual void Initialize(Object3D* model, Camera* camera, const Vector3& position, const GameObjectInitConfig& config);
    virtual void Update() override; // 派生クラスで固有のAIを実装
    virtual void Draw();

    void UpdateRespawn(float deltaTime, float respawnTime, bool isEnemyRespawnEnabled);

    // コライダーの衝突時用
    virtual void OnCollision() {} 

    // ゲッター/セッター
    bool GetIsLockedOn() const { return isLockedOn_; }
    void SetIsLockedOn(bool flag);

    bool GetIsDead() const { return isDead_; }
    void SetIsDead(bool flag);

    bool ConsumeDefeatEffectRequest();

    void SetPlayer(Player* player) { player_ = player; }
    Player* GetPlayer() const { return player_; }

    int GetExpReward() const { return expReward_; }
    void SetExpReward(int exp) { expReward_ = exp; }

    float GetSearchRadius() const { return searchRadius_; }
    void SetSearchRadius(float radius) { searchRadius_ = radius; }

    float GetLoseRadius() const { return loseRadius_; }
    void SetLoseRadius(float radius) { loseRadius_ = radius; }

    Vector3 GetWorldPosition() const { return worldTransform_.translate; }

    virtual void DrawImGui(int index) = 0; // ImGui表示

protected:
    Player* player_ = nullptr;
    float searchRadius_ = 10.0f;
    float loseRadius_ = 12.0f;

    bool isLockedOn_ = false;
    bool defeatEffectRequested_ = false;
    Vector3 spawnPosition_{};
    float respawnTimer_ = 0.0f;

    int expReward_ = 30;

    // 弾関連の共通メンバ
    std::list<std::unique_ptr<EnemyBullet>> bullets_;
    static inline const float kShootInterval = 2.0f;
    static inline const float kBulletSpeed = 0.15f;
    float shootTimer_ = 0.0f;

    // チャージ関連
    bool isCharging_ = false;
    float chargeTimer_ = 0.0f;
    float uvScrollTimer_ = 0.0f;
    static inline const float kChargeTime = 0.75f;
    static inline const float kMaxChargeRingScale = 3.0f;

    // アニメーション関連
    float walkTimer_ = 0.0f;
    static inline const float kWalkTimer = 2.0f;
    static inline const float kWalkMotionAngleStart = -25.0f;
    static inline const float kWalkMotionAngleEnd = 50.0f;
    static inline const float kPi = 3.14159265359f;

    LRDirection chasingDirection_ = LRDirection::kLeft;
};

}
