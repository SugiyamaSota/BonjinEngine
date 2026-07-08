#pragma once
#include <cassert>
#include <list>
#include <memory>

#include "../anchor/anchor.h"
#include "../../logic/Data.h"
#include "../BaseCharacter.h"

#include "Object3D.h"
#include "Line3D.h"

class MapChipField;
class Enemy;
class Camera;

/// <summary>
/// 自キャラ
/// </summary>
class Player : public BaseCharacter {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Object3D* model, Camera* camera, const Vector3& position);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();


	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// アンカーラインの描画処理（描画順序を最後に調整するため個別化）
	/// </summary>
	void DrawAnchorLine();

	/// <summary>
	/// 移動処理
	/// </summary>
	void Move();

protected:
	void OnMapCollision(const CollisionMapInfo& collisionMapinfo) override;

public:
	Vector3 GetWorldPosition();
	AABB GetAABB();
	void OnCollision(Enemy* enemy);
	bool ConsumeLandingEffectRequest();
	bool HasAnchor() const {
		return anchor_ != nullptr;
	}

	Anchor& GetAnchor() {
		assert(anchor_); // nullではないことを確認
		return *anchor_;
	}

	/// <summary>
	/// アンカーのゲッター (const版)
	/// </summary>
	const Anchor& GetAnchor() const {
		assert(anchor_); // nullではないことを確認
		return *anchor_;
	}

	void SetLockedOnEnemiesList(std::list<Enemy*>* enemiesList) {
		lockedOnEnemies_ = enemiesList;
	}

	// HP関連
	int GetHp() const { return hp_; }
	void SetHp(int hp) { hp_ = hp; }
	void ApplyDamage(int damage) { hp_ = (hp_ > damage) ? hp_ - damage : 0; }
	bool GetIsDead() const { return hp_ <= 0; }

	bool GetIsInvincible() const { return isInvincible_; }

	// ロックオン中の敵をすべて削除する処理（入力判定を含む)
	void HandleLockOnRemovalInput();

	// ロックオン中の敵をすべて倒す
	void RemoveLockedOnEnemies(std::list<Enemy*>& enemies);

	void DrawImGui();

	void UpdateWorldTransform(){ model_->Update(worldTransform_, camera_); }

private:
	static inline const float kAcceleration = 0.010f;
	static inline const float kAttenuation = 0.8f;
	static inline const float kLimitRunSpeed = 0.15f;

	static inline const float kAccelerationInAir = 0.010f;

	//旋回関連
	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;
	static inline const float kTimeTurn = 0.3f;

	//ジャンプ初速
	static inline const float kJumpAcceleration = 0.32f;

	// --- 当たり判定 ---
	// 衝突後処理の数値
	static inline const float kAttenuationTop = 0.5f;
	static inline const float kAttenuationWall = 0.5f;

	static inline const float kKnockbackUpPower = 0.15f;
	static inline const float kKnockbackPower = 0.15f;

	bool landingEffectRequested_ = false;

	bool isKnockedBack_ = false;
	float knockbackTimer_ = 0.0f;
	static inline const float kKnockbackTime = 1.0f;
	static inline const float kKnockbackAttenuation = 0.95f;

	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;
	static inline const float kInvincibleTime = 1.5f;

	static inline const float kWidth = 1.9f;
	static inline const float kHeight = 1.9f;

	// Padのデッドゾーン
	long kPadDeadZone_ = 750;

	// --- アンカー関連 ---
	float kAnchorSpeed = 0.1f; // アンカーの初速の大きさ
	static inline const long kAnchorDeadZone = 750; // アンカー発射時のスティック入力のデッドゾーン

	// --- アンカー ---
	std::unique_ptr<Anchor> anchor_;
	std::unique_ptr<Bonjin::Line3D> anchorLine_;
	Vector4 lineColor_ = { 0.5f, 0.85f, 1.f, 0.5f };
	/// <summary>
	/// アンカーの射出
	/// </summary>
	void shootAnchor();

	// ロックオンされた敵のリストへのポインタ
	std::list<Enemy*>* lockedOnEnemies_ = nullptr;

	// HP
	int hp_ = 3;
};
