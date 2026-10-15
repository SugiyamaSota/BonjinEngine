#pragma once
#include "BaseEnemy.h"

namespace Bonjin {

enum class EnemyState {
	kPatrol, // 巡回
	kChase,  // 追跡
};

class Enemy : public BaseEnemy {
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
    EnemyState state_ = EnemyState::kPatrol; // 行動状態
    static inline const float kWalkSpeed = 0.05f; // 歩行速度

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
