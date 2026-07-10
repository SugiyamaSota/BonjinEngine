#pragma once
#include "../logic/Data.h"
#include "Object3D.h"
#include "../logic/Collider.h"
#include "../logic/CollisionConfig.h"
#include <memory>
#include <string>

class MapChipField;
class Camera;

namespace Bonjin {

enum class PhysicsType {
	None,     // 物理更新なし
	Linear,   // 直線運動（重力なし）
	Gravity,  // 重力あり
};

struct GameObjectInitConfig {
	PhysicsType physicsType = PhysicsType::None;
	bool hasCollider = false;
	uint32_t categoryAttr = 0;
	uint32_t collisionMask = 0;
	std::string tag = "";
};

class GameObject {
protected:
	// ワールドトランスフォーム
	WorldTransform worldTransform_{};

	// カメラ
	Camera* camera_ = nullptr;

	// 3Dモデル
	Object3D* model_ = nullptr;

	// 移動速度
	Vector3 velocity_ = {};

	// マップチップフィールド
	MapChipField* mapChipField_ = nullptr;

	// キャラクターのサイズ
	float width_ = 1.0f;
	float height_ = 1.0f;

	// 接地状態フラグ
	bool onGround_ = false;

	// 向き
	LRDirection lrDirection_ = LRDirection::kRight;

	// 物理設定
	PhysicsType physicsType_ = PhysicsType::None;

	// コライダー
	std::unique_ptr<Collider> collider_;

	// 死亡フラグ
	bool isDead_ = false;

	// 物理設定の定数
	static inline const float kGravityAcceleration = 0.0098f;
	static inline const float kLimitFallSpeed = 2.0f;

public:
	virtual ~GameObject() = default;

	// 初期化
	virtual void Initialize(Object3D* model, Camera* camera, const Vector3& position, const GameObjectInitConfig& config);
	
	// 共通の更新処理
	virtual void Update();

	// 共通の描画
	virtual void Draw() const;

	// ゲッター・セッター
	WorldTransform& GetWorldTransform() { return worldTransform_; }
	const Vector3& GetVelocity() const { return velocity_; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
	Vector3 GetPosition() const { return worldTransform_.translate; }
	void SetPosition(const Vector3& position) { worldTransform_.translate = position; }
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	bool IsOnGround() const { return onGround_; }
	bool IsDead() const { return isDead_; }
	void SetDead(bool flag) { isDead_ = flag; }
	void SetWidth(float w) { width_ = w; }
	void SetHeight(float h) { height_ = h; }

	Collider* GetCollider() const { return collider_.get(); }
	AABB GetAABB() const;

protected:
	// 共通の物理・マップ衝突判定更新
	void UpdatePhysicsAndMapCollision();

	// 衝突時のコールバック
	virtual void OnMapCollision(const CollisionMapInfo& collisionMapInfo);

	// オブジェクト同士の衝突時コールバック
	virtual void OnCollision(Collider* other) {}
};

}
