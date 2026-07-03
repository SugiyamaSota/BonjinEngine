#pragma once
#include "../logic/Data.h"
#include "Object3D.h"

class MapChipField;
class Camera;

class BaseCharacter {
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

	// 物理設定の定数
	static inline const float kGravityAcceleration = 0.0098f;
	static inline const float kLimitFallSpeed = 2.0f;

public:
	virtual ~BaseCharacter() = default;

	// ゲッター・セッター
	WorldTransform& GetWorldTransform() { return worldTransform_; }
	const Vector3& GetVelocity() const { return velocity_; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
	Vector3 GetPosition() const { return worldTransform_.translate; }
	void SetPosition(const Vector3& position) { worldTransform_.translate = position; }
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	bool IsOnGround() const { return onGround_; }

protected:
	// 共通の物理・マップ衝突判定更新
	void UpdatePhysicsAndMapCollision();

	// 衝突時のコールバック（各キャラで挙動をカスタマイズするため）
	virtual void OnMapCollision(const CollisionMapInfo& collisionMapInfo);
};
