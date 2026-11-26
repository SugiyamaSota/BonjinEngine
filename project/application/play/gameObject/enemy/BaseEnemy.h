#pragma once
#include "../../logic/Data.h"
#include "../GameObject.h"
#include "EnemyBullet.h"
#include "Object3D.h"
#include "DebugCircle3D.h"
#include <list>
#include <memory>
#include <string>

class Player;

namespace Bonjin {

/// <summary>
/// 敵の基底クラス
/// 1フレームの流れ (状態遷移 -> 巡回/追跡 -> 弾 -> 移動 -> 向き -> モーション) はここで決め、
/// 敵の種類ごとに違う部分だけを派生クラスで上書きする (Template Method)
/// </summary>
class BaseEnemy : public GameObject {
public:
    virtual ~BaseEnemy() = default;

    /// <summary>
    /// 初期化 (種類ごとの設定は派生クラスで決める)
    /// </summary>
    virtual void Initialize(Object3D* model, Camera* camera, const Vector3& position) = 0;

    /// <summary>
    /// 更新 (流れは全ての敵で共通。派生クラスでは上書きしない)
    /// </summary>
    void Update() final;

    void Draw() const override;

    bool IsChasing() const { return state_ == State::kChase; }

    void UpdateRespawn(float deltaTime, float respawnTime, bool isEnemyRespawnEnabled);

    /// <summary>
    /// アンカーが当たったとき (ロックオンされる)
    /// </summary>
    void OnAnchorHit() { SetIsLockedOn(true); }

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
    // 行動状態
    enum class State {
        kPatrol, // 巡回
        kChase,  // 追跡
    };

    /// <summary>
    /// 共通の初期化 (派生クラスの Initialize から呼ぶ)
    /// </summary>
    void Initialize(Object3D* model, Camera* camera, const Vector3& position, const GameObjectInitConfig& config) override;

    // --- 敵の種類ごとに違う部分 (派生クラスで上書きする) ---

    // 巡回中の行動
    virtual void PatrolBehavior() = 0;
    // 追跡中の行動
    virtual void ChaseBehavior() = 0;
    // 歩く・浮くなどの見た目の揺れ
    virtual void UpdateMotion() = 0;
    // 追跡を始めたとき
    virtual void OnStartChase() {}
    // 追跡をやめて巡回に戻ったとき
    virtual void OnLoseChase() = 0;
    // ふだんの色 (ロックオンが外れたとき・リスポーンしたときもこの色に戻す)
    virtual Vector4 GetBaseColor() const = 0;
    // 弾を撃つ前のチャージリングの色
    virtual Vector4 GetChargeRingColor() const = 0;

    // --- 派生クラスから使う共通処理 ---

    // プレイヤーの方を向き、一定間隔でチャージを始める (追跡中に呼ぶ)
    void UpdateChaseAttack();

    // ImGuiのツリーの名前 ("<name> 0 [CHASE]" など)
    std::string MakeImGuiNodeName(const char* name, int index) const;

    State state_ = State::kPatrol;

    Player* player_ = nullptr;
    float searchRadius_ = 5.0f;
    float loseRadius_ = 12.0f;

    std::unique_ptr<DebugCircle3D> searchRangeCircle_;

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

private:
    // 巡回 <-> 追跡 の切り替え
    void UpdateStateTransition();
    // プレイヤーが直視できる位置にいるか (壁越しの場合はfalse)
    bool IsPlayerVisible() const;
    // チャージと弾の更新
    void UpdateChargeAndBullets();
    // モデルの向きを左右の向きに合わせる
    void TurningControl();
};

}
