#pragma once
#include "../../logic/Data.h"
#include "../BaseCharacter.h"
#include "EnemyBullet.h"

#include "Object3D.h"
#include "Sprite.h"

class MapChipField;
class Player;

enum class EnemyState {
	kPatrol, // 巡回
	kChase,  // 追跡
};

class Enemy : public BaseCharacter {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="model">使用する3Dモデル</param>
    /// <param name="camera">描画に使用するカメラ</param>
    /// <param name="position">初期座標</param>
    void Initialize(Object3D* model, Camera* camera, const Vector3& position);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();
    void UpdateRespawn(float deltaTime, float respawnTime, bool isRespawnEnabled);

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// 当たり判定時の処理
    /// </summary>
    void OnCollision();

    /// <summary>
    /// ワールド座標を取得
    /// </summary>
    /// <returns>ワールド座標</returns>
    Vector3 GetWorldPosition() const;

    /// <summary>
    /// AABB(Axis-Aligned Bounding Box)を取得
    /// </summary>
    /// <returns>AABB</returns>
    AABB GetAABB();

    /// <summary>
    /// ロックオン状態を取得
    /// </summary>
    /// <returns>ロックオン中であればtrue</returns>
    bool GetIsLockedOn() const { return isLockedOn_; }

    /// <summary>
    /// 死亡状態を取得
    /// </summary>
    /// <returns>死亡していればtrue</returns>
    bool GetIsDead() const { return isDead_; }
    bool ConsumeDefeatEffectRequest();

    // セッター
    /// <summary>
    /// ロックオン状態を設定
    /// </summary>
    /// <param name="flag">設定するフラグ</param>
    void SetIsLockedOn(bool flag);

    /// <summary>
    /// 死亡状態を設定
    /// </summary>
    /// <param name="flag">設定するフラグ</param>
    void SetIsDead(bool flag);

    /// <summary>
    /// プレイヤーを設定
    /// </summary>
    void SetPlayer(Player* player) { player_ = player; }

    /// <summary>
    /// 状態ゲッター
    /// </summary>
    EnemyState GetState() const { return state_; }

    /// <summary>
    /// プレイヤーゲッター
    /// </summary>
    Player* GetPlayer() const { return player_; }

    /// <summary>
    /// 検知距離ゲッターセッター
    /// </summary>
    float GetSearchRadius() const { return searchRadius_; }
    void SetSearchRadius(float radius) { searchRadius_ = radius; }

    /// <summary>
    /// 見失い距離ゲッターセッター
    /// </summary>
    float GetLoseRadius() const { return loseRadius_; }
    void SetLoseRadius(float radius) { loseRadius_ = radius; }

protected:
    void OnMapCollision(const CollisionMapInfo& collisionMapinfo) override;

private:
    // --- メンバー変数 ---

    // 状態
    EnemyState state_ = EnemyState::kPatrol; // 行動状態
    Player* player_ = nullptr;               // プレイヤーへのポインタ
    float searchRadius_ = 10.0f;             // プレイヤー探知距離
    float loseRadius_ = 12.0f;               // プレイヤー見失い距離

    bool isLockedOn_ = false;         // ロックオン状態
    bool isDead_ = false;             // 死亡状態
    bool defeatEffectRequested_ = false; // 撃破エフェクトの発生要求
    Vector3 spawnPosition_{};             // リスポーン地点
    float respawnTimer_ = 0.0f;           // 死亡後の経過時間

    // 当たり判定
    static inline const float kWidth_ = 2.0f;     // 幅
    static inline const float kHeight_ = 2.0f;    // 高さ

    // 移動
    static inline const float kWalkSpeed = 0.05f; // 歩行速度

    // アニメーション
    float walkTimer_ = 0.0f;                       // 歩行アニメーションのタイマー
    static inline const float kWalkTimer = 2.0f;   // 歩行アニメーションの周期
    static inline const float kWalkMotionAngleStart = -25.0f; // 首を振る開始角度
    static inline const float kWalkMotionAngleEnd = 50.0f;    // 首を振る終了角度
    static inline const float kPi = 3.14159265359f; // 円周率

    LRDirection chasingDirection_ = LRDirection::kLeft; // モデルの向き

    // 弾関連
    std::list<std::unique_ptr<EnemyBullet>> bullets_;
    static inline const float kShootInterval = 2.0f; // 発射間隔（秒）
    static inline const float kBulletSpeed = 0.15f;  // 弾速
    float shootTimer_ = 0.0f;

private:
    // --- プライベート関数 ---

    /// <summary>
    /// モデルの向きを制御
    /// </summary>
    void TurningControl();

    /// <summary>
    /// プレイヤーが直視できる位置にいるか判定 (壁越しの場合はfalse)
    /// </summary>
    bool IsPlayerVisible() const;

    /// <summary>
    /// 状態遷移の更新
    /// </summary>
    void UpdateStateTransition();

    /// <summary>
    /// 巡回行動
    /// </summary>
    void PatrolBehavior();

    /// <summary>
    /// 追跡行動
    /// </summary>
    void ChaseBehavior();

    /// <summary>
    /// 物理と移動の適用
    /// </summary>
    void ApplyPhysicsAndMovement();
};
