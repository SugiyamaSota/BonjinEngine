#pragma once
#include "BaseEnemy.h"

namespace Bonjin {

/// <summary>
/// 空中の敵 (重力なし。巡回中はふわふわ飛び、追跡中はプレイヤーのまわりを円を描いて回りながら撃つ)
/// </summary>
class NoGravityEnemy : public BaseEnemy {
public:
    /// <summary>
    /// 初期化 (positionはマップチップの中心。少し高めに出現する)
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
    void OnStartChase() override;
    void OnLoseChase() override;
    Vector4 GetBaseColor() const override { return { 0.2f, 0.3f, 0.5f, 1.0f }; }        // 青みがかった色
    Vector4 GetChargeRingColor() const override { return { 0.0f, 0.5f, 1.0f, 0.5f }; }  // 青の半透明

private:
    static inline const float kFlySpeed = 0.04f;     // 飛行速度
    static inline const float kSpawnHeight = 1.0f;   // マップチップの中心からどれだけ高く出現するか

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
};

}
