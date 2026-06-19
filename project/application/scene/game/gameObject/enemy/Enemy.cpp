#include "Enemy.h"
#include "../../logic/Collision.h"
#include "../../mapchip/MapChipField.h"
#include <cmath>
#include <numbers>
#include <random>
#include <list>
#include <algorithm>

using namespace Bonjin;

void Enemy::Initialize(Object3D* model, Camera* camera, const Vector3& position) {

	worldTransform_ = {
		{1,1,1},
		{0,0,0},
		position,
	};

	model_ = model;

	camera_ = camera;

	velocity_ = { -kWalkSpeed, 0, 0 };

	walkTimer_ = 0.0f;
	isLockedOn_ = false;
	isDead_ = false;
	defeatEffectRequested_ = false;

	/*
	lockedOnSprite_ = new Sprite();
	lockedOnSprite_->Initialize("resources/textures/default.png");*/

	/*Vector3 worldPos = GetWorldPosition();
	Vector3 screenPos = camera_->Project(worldPos);*/
	//lockedOnSprite_->Translate() = { screenPos.x,screenPos.y };
}

void Enemy::Update() {

	// プレイヤーと共通のマップ衝突処理を使って移動する。
	velocity_.y -= kGravityAcceleration;
	if (velocity_.y < -kLimitFallSpeed) {
		velocity_.y = -kLimitFallSpeed;
	}
	if (mapChipField_) {
		const CollisionMapInfo collisionInfo =
			ResolveMapCollision(*mapChipField_, worldTransform_.translate, velocity_, kWidth_, kHeight_);
		worldTransform_.translate = Add(worldTransform_.translate, collisionInfo.movement_);

		if (collisionInfo.isLandin_ || collisionInfo.isHotTop_) {
			velocity_.y = 0.0f;
		}
		if (collisionInfo.isHitWall_) {
			velocity_.x *= -1.0f;
			lrDirection_ =
				velocity_.x > 0.0f ? LRDirection::kRight : LRDirection::kLeft;
		}
	} else {
		worldTransform_.translate = Add(worldTransform_.translate, velocity_);
	}

	// 旋回制御
	TurningControl();

	// 縦方向の動き
	walkTimer_ += 1.0f / 60.0f;

	float param = std::sin(kPi * 2 * walkTimer_ / kWalkTimer);
	float radian = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;
	worldTransform_.rotate.z = radian * (kPi / 180.0f);

	if (walkTimer_ >= kWalkTimer) {
		walkTimer_ = 0.0f;
	}

	// 行列の変換
	model_->Update(worldTransform_, camera_);

    //lockedOnSprite_->Update();
}


void Enemy::Draw() { 
	model_->Draw(); 
	
	
}

void Enemy::TurningControl() {
	{
		float destinationRotationYTable[] = {
			std::numbers::pi_v<float> *-4.0f / 2.0f,
			std::numbers::pi_v<float> *2.0f / 2.0f,
		};
		// 状況に応じた角度を取得する
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		// 自キャラの角度を設定する
		worldTransform_.rotate.y = destinationRotationY;
	}
}

Vector3 Enemy::GetWorldPosition() {
	return worldTransform_.translate;
}


AABB Enemy::GetAABB() {
	AABB aabb;
	Vector3 worldPos = GetWorldPosition();
	aabb.min = { worldPos.x - kWidth_ / 2.0f, worldPos.y - kHeight_ / 2.0f, worldPos.z - kWidth_ / 2.0f };
	aabb.max = { worldPos.x + kWidth_ / 2.0f, worldPos.y + kHeight_ / 2.0f, worldPos.z + kWidth_ / 2.0f };
	return aabb;
}

void Enemy::OnCollision() {
	SetIsLockedOn(true);
}

void Enemy::SetIsLockedOn(bool frag) {
	isLockedOn_ = frag;
	if (frag == true) {
		model_->SetColor(Vector4{ 1, 0, 0, 1 });
	} else {
		model_->SetColor(Vector4{ 1,1,1,1 });
	}
}

void Enemy::SetIsDead(bool flag) {
	if (flag && !isDead_) {
		defeatEffectRequested_ = true;
	}
	isDead_ = flag;
}

bool Enemy::ConsumeDefeatEffectRequest() {
	if (!defeatEffectRequested_) {
		return false;
	}

	defeatEffectRequested_ = false;
	return true;
}
