#pragma once
#include "BaseEnemy.h"

namespace Bonjin {

/// <summary>
/// 地上の敵 (重力あり。巡回中は歩き、追跡中は立ち止まって撃つ)
/// </summary>
class Enemy : public BaseEnemy {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(Object3D* model, Camera* camera, const Vector3& position) override;

    /// <summary>
    /// ImGui描画処理
    /// </summary>
    void DrawImGui(int index) override;

protected:
    void OnMapCollision(const CollisionMapInfo& collisionMapinfo) override;

    void PatrolBehavior() override;
    void ChaseBehavior() override;
    void UpdateMotion() override;
    void OnLoseChase() override;
    Vector4 GetBaseColor() const override { return { 0.3f, 0.2f, 0.2f, 1.0f }; }        // グレーがかった赤
    Vector4 GetChargeRingColor() const override { return { 1.0f, 0.0f, 0.0f, 0.5f }; }  // 赤の半透明

private:
    static inline const float kWalkSpeed = 0.05f; // 歩行速度
};

}
