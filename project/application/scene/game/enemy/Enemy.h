#pragma once
#include "BonjinEngine.h"

#include "../others/Data.h"
#include "Debris.h"

class Enemy {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="model">使用する3Dモデル</param>
    /// <param name="camera">描画に使用するカメラ</param>
    /// <param name="position">初期座標</param>
    void Initialize(Model* model, Camera* camera, const Vector3& position);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

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
    void SetIsDead(bool flag) { isDead_ = flag; }

    /// <summary>
   /// 爆発して破片を取得
   /// </summary>
   /// <returns>生成された破片のリスト</returns>
    std::list<std::unique_ptr<Debris>> ExplodeAndGetDebris();

private:
    // --- メンバー変数 ---

    // 共通
    WorldTransform worldTransform_{}; // ワールドトランスフォーム
    Camera* camera_ = nullptr;        // カメラ
    Model* model_ = nullptr;          // 3Dモデル

    // 状態
    bool isLockedOn_ = false;         // ロックオン状態
    bool isDead_ = false;             // 死亡状態

    // 当たり判定
    static inline const float kWidth_ = 2.0f;     // 幅
    static inline const float kHeight_ = 2.0f;    // 高さ

    // 移動
    Vector3 velocity_ = {};                       // 現在の速度
    static inline const float kWalkSpeed = 0.05f; // 歩行速度

    // アニメーション
    LRDirection lrDirection_ = LRDirection::kLeft; // モデルの向き
    float walkTimer_ = 0.0f;                       // 歩行アニメーションのタイマー
    static inline const float kWalkTimer = 2.0f;   // 歩行アニメーションの周期
    static inline const float kWalkMotionAngleStart = -25.0f; // 首を振る開始角度
    static inline const float kWalkMotionAngleEnd = 50.0f;    // 首を振る終了角度
    static inline const float kPi = 3.14159265359f; // 円周率

    Bonjin::Sprite* lockedOnSprite_ = nullptr;

private:
    // --- プライベート関数 ---

    /// <summary>
    /// モデルの向きを制御
    /// </summary>
    void TurningControl();
};