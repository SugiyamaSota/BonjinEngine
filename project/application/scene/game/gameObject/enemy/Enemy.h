#pragma once
#include "../../logic/Data.h"

#include "Object3D.h"
#include "Sprite.h"

class MapChipField;

class Enemy {
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
    Vector3 GetWorldPosition();

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
    /// 衝突判定に使用するマップを設定
    /// </summary>
    void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

private:
    // --- メンバー変数 ---

    // 共通
    WorldTransform worldTransform_{}; // ワールドトランスフォーム
    Camera* camera_ = nullptr;        // カメラ
    Object3D* model_ = nullptr;        // 3Dモデル
    MapChipField* mapChipField_ = nullptr; // マップチップフィールド

    // 状態
    bool isLockedOn_ = false;         // ロックオン状態
    bool isDead_ = false;             // 死亡状態
    bool defeatEffectRequested_ = false; // 撃破エフェクトの発生要求
    Vector3 spawnPosition_{};             // リスポーン地点
    float respawnTimer_ = 0.0f;           // 死亡後の経過時間

    // 当たり判定
    static inline const float kWidth_ = 2.0f;     // 幅
    static inline const float kHeight_ = 2.0f;    // 高さ

    // 移動
    Vector3 velocity_ = {};                       // 現在の速度
    static inline const float kWalkSpeed = 0.05f; // 歩行速度
    static inline const float kGravityAcceleration = 0.0098f;
    static inline const float kLimitFallSpeed = 2.0f;

    // アニメーション
    LRDirection lrDirection_ = LRDirection::kLeft; // モデルの向き
    float walkTimer_ = 0.0f;                       // 歩行アニメーションのタイマー
    static inline const float kWalkTimer = 2.0f;   // 歩行アニメーションの周期
    static inline const float kWalkMotionAngleStart = -25.0f; // 首を振る開始角度
    static inline const float kWalkMotionAngleEnd = 50.0f;    // 首を振る終了角度
    static inline const float kPi = 3.14159265359f; // 円周率

    //Bonjin::Sprite* lockedOnSprite_ = nullptr;

private:
    // --- プライベート関数 ---

    /// <summary>
    /// モデルの向きを制御
    /// </summary>
    void TurningControl();
};
