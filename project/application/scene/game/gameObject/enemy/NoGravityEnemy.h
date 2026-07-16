#pragma once
#include "BaseEnemy.h"

namespace Bonjin {

enum class NoGravityEnemyState {
	kPatrol, // 巡回
	kChase,  // 追跡
};

class NoGravityEnemy : public BaseEnemy {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(Object3D* model, Camera* camera, const Vector3& position);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// ImGui描画処理
    /// </summary>
    void DrawImGui(int index) override;

    /// <summary>
    /// 当たり判定時の処理
    /// </summary>
    void OnCollision() override;

    bool IsChasing() const override;

protected:
    void OnMapCollision(const CollisionMapInfo& collisionMapinfo) override;

private:
    // --- メンバー変数 ---

    // 状態
    NoGravityEnemyState state_ = NoGravityEnemyState::kPatrol; // 行動状態
    static inline const float kFlySpeed = 0.04f; // 飛行速度

    // ふわふわ浮遊運動用
    float floatingTimer_ = 0.0f;
    static inline const float kFloatSpeed = 3.0f; // ふわふわ動く速度
    static inline const float kFloatAmplitude = 0.4f; // ふわふわ揺れる振幅
    float patrolBaseY_ = 0.0f; // パトロール時の基準Y座標

    // 円運動追跡用
    float chaseAngle_ = 0.0f;          // 現在の角度
    float orbitDirection_ = 1.0f;      // 回転方向 (1.0f: 反時計回り, -1.0f: 時計回り)
    bool isChaseInitialized_ = false;  // 追跡開始時の初期角度設定用フラグ

    // 円運動のパラメータ
    float orbitRadius_ = 4.0f;         // 半径
    float orbitSpeed_ = 1.2f;          // 速度
    float minOrbitAngle_ = 0.0f;       // 最小角度制限 (真右)
    float maxOrbitAngle_ = 3.14159f;   // 最大角度制限 (真左)

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
};

}
