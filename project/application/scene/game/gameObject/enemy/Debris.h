#pragma once
#include "../../engine/bonjin/BonjinEngine.h"

class Debris {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="model">使用する3Dモデル</param>
    /// <param name="camera">描画に使用するカメラ</param>
    /// <param name="position">初期座標</param>
    /// <param name="velocity">初期速度</param>
    void Initialize(Camera* camera, const Vector3& position, const Vector3& velocity);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// 死亡状態を取得
    /// </summary>
    /// <returns>死亡していればtrue</returns>
    bool GetIsDead() const { return isDead_; }

private:
    // --- メンバー変数 ---

    WorldTransform worldTransform_{}; // ワールドトランスフォーム
    Camera* camera_ = nullptr;        // カメラ
    Model* model_ = nullptr;          // 3Dモデル

    Vector3 velocity_ = {};            // 現在の速度
    float rotationSpeed_ = 0.0f;       // 回転速度
    float timer_ = 0.0f;               // 経過時間
    bool isDead_ = false;              // 死亡状態

    // --- 定数 ---
    static inline const float kGravity = 9.8f;      // 重力加速度
    static inline const float kDebrisLifeTime = 3.0f; // 破片の生存時間
};