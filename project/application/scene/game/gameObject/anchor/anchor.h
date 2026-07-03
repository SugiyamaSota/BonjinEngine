#pragma once

#include "Object3D.h"

#include"../../logic/Data.h"
#include "../../mapchip/MapChipField.h" // マップチップフィールドをインクルード

class Anchor {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="position">初期座標</param>
    /// <param name="velocity">初期速度</param>
    /// <param name="mapChipField">マップチップクラス</param>
    Anchor(const Vector3& position, const Vector3& velocity,MapChipField* mapChipField);

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~Anchor();

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <param name="camera">カメラクラス</param>
    void Update(Camera& camera);

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw() const;

    // 当たり判定メソッドを追加
    bool isCollisionMap();

    bool GetStandBy()const { return isStandBy; }
    void SetStandBy(bool flag) { isStandBy = flag; }

    Vector3& GetPosition(){ return worldTransform_.translate; }
    void SetPosition(const Vector3& pos) { worldTransform_.translate = pos; }

    const Vector3& GetVelocity() const { return velocity_; }
    Vector3& GetVelocity() { return velocity_; }
    float GetAngle() const { return worldTransform_.rotate.z; }
    AABB GetAABB();
    void OnCollision();
    bool IsDead() const { return isDead_; }
    void SetDead(bool flag) { isDead_ = flag; }
    bool IsInstantResolved() const { return isInstantResolved_; }
    void SetInstantResolved(bool flag) { isInstantResolved_ = flag; }
private:
    // 座標
    Vector3 position_;
    // 速度
    Vector3 velocity_;

    // モデル
    Object3D* model_;
    // ワールドトランスフォーム
    WorldTransform worldTransform_;

    // マップチップクラス
    MapChipField* mapChipField_ = nullptr;

    // 当たり判定サイズ
    static inline const float kWidth = 1.0f;
    static inline const float kHeight = 1.0f;

    enum Corner {
        kRightBottom,
        kLeftBottom,
        kRightTop,
        kLeftTop,
        kNumCorner
    };

    // 角の座標を取得するヘルパーメソッド
    Vector3 CornerPosition(const Vector3& center, Corner corner);

    // 待機状態
    bool isStandBy;

    // 破壊
    bool isDead_ = false;
    bool isInstantResolved_ = false;

    Vector3 GetWorldPosition();

};
