#include "Enemy.h"
#include "../../logic/Collision.h"
#include "../../mapchip/MapChipField.h"
#include "../player/Player.h"
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
	spawnPosition_ = position;

	model_ = model;

	camera_ = camera;

	velocity_ = { -kWalkSpeed, 0, 0 };

	walkTimer_ = 0.0f;
	isLockedOn_ = false;
	isDead_ = false;
	defeatEffectRequested_ = false;
	respawnTimer_ = 0.0f;
	state_ = EnemyState::kPatrol;
	player_ = nullptr;
	lrDirection_ = LRDirection::kLeft;

	width_ = kWidth_;
	height_ = kHeight_;

	shootTimer_ = 0.0f;
	bullets_.clear();
}

void Enemy::Update() {
	if (player_) {
		UpdateStateTransition();
	}

	switch (state_) {
	case EnemyState::kPatrol:
		PatrolBehavior();
		break;
	case EnemyState::kChase:
		ChaseBehavior();
		break;
	}

	ApplyPhysicsAndMovement();

	// 弾の更新
	for (auto it = bullets_.begin(); it != bullets_.end();) {
		(*it)->Update();

		// プレイヤーとの衝突判定
		if (!(*it)->IsDead() && player_ && !player_->GetIsDead()) {
			if (IsCollision((*it)->GetAABB(), player_->GetAABB())) {
				(*it)->SetDead(true);
				player_->ApplyDamage(1);
			}
		}

		if ((*it)->IsDead()) {
			it = bullets_.erase(it);
		} else {
			++it;
		}
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

bool Enemy::IsPlayerVisible() const {
	if (!mapChipField_ || !player_) {
		return false;
	}

	Vector3 start = GetWorldPosition();
	Vector3 end = player_->GetWorldPosition();

	Vector3 direction = Subtract(end, start);
	float distance = Length(direction);
	if (distance <= 0.05f) {
		return true;
	}

	Vector3 dirNorm = Normalize(direction);
	
	const float step = 0.5f;
	float currentDistance = step;

	while (currentDistance < distance) {
		Vector3 currentPos = Add(start, Multiply(currentDistance, dirNorm));
		IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(currentPos);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);

		if (type == MapChipType::kBlock) {
			return false;
		}

		currentDistance += step;
	}

	return true;
}

void Enemy::UpdateStateTransition() {
	Vector3 playerPos = player_->GetWorldPosition();
	Vector3 myPos = GetWorldPosition();
	
	float distance = Length(Subtract(playerPos, myPos));

	if (state_ == EnemyState::kPatrol) {
		if (distance <= searchRadius_ && IsPlayerVisible()) {
			chasingDirection_ = lrDirection_;
			state_ = EnemyState::kChase;
		}
	} else if (state_ == EnemyState::kChase) {
		if (distance > loseRadius_ || !IsPlayerVisible()) {
			state_ = EnemyState::kPatrol;
			lrDirection_ = chasingDirection_;
			if (lrDirection_ == LRDirection::kRight) {
				velocity_.x = kWalkSpeed;
			} else {
				velocity_.x = -kWalkSpeed;
			}
		}
	}
}

void Enemy::PatrolBehavior() {
	// 巡回時は何もしない（ApplyPhysicsAndMovementで反転処理などを行う）
}

void Enemy::ChaseBehavior() {
	Vector3 playerPos = player_->GetWorldPosition();
	Vector3 myPos = GetWorldPosition();

	velocity_.x = 0.f;

	if (playerPos.x < myPos.x) {
		//velocity_.x = -kWalkSpeed;
		lrDirection_ = LRDirection::kLeft;
	} else {
		//velocity_.x = kWalkSpeed;
		lrDirection_ = LRDirection::kRight;
	}

	// 追跡中、一定間隔で弾を発射
	shootTimer_ -= 1.0f / 60.0f; // 60fps想定
	if (shootTimer_ <= 0.0f) {
		Vector3 direction = Subtract(playerPos, myPos);
		if (Length(direction) > 0.05f) {
			Vector3 bulletVelocity = Multiply(kBulletSpeed, Normalize(direction));
			bulletVelocity.z = 0.0f; // 2D平面上の動きにするためZ軸は固定
			bullets_.push_back(std::make_unique<EnemyBullet>(myPos, bulletVelocity, camera_, mapChipField_));
		}
		shootTimer_ = kShootInterval;
	}
}

void Enemy::ApplyPhysicsAndMovement() {
	UpdatePhysicsAndMapCollision();
}


void Enemy::Draw() { 
	model_->Draw(); 

	// 弾の描画
	for (auto& bullet : bullets_) {
		bullet->Draw();
	}
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

Vector3 Enemy::GetWorldPosition() const {
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

void Enemy::UpdateRespawn(float deltaTime, float respawnTime, bool isRespawnEnabled) {
	if (!isDead_) {
		return;
	}

	if (!isRespawnEnabled) {
		respawnTimer_ = 0.0f;
		return;
	}

	respawnTimer_ += deltaTime;
	if (respawnTimer_ < respawnTime) {
		return;
	}

	worldTransform_ = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		spawnPosition_,
	};
	velocity_ = {-kWalkSpeed, 0.0f, 0.0f};
	lrDirection_ = LRDirection::kLeft;
	walkTimer_ = 0.0f;
	respawnTimer_ = 0.0f;
	isDead_ = false;
	isLockedOn_ = false;
	defeatEffectRequested_ = false;
	state_ = EnemyState::kPatrol;
	shootTimer_ = 0.0f;
	bullets_.clear();
	model_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	model_->Update(worldTransform_, camera_);
}

void Enemy::SetIsDead(bool flag) {
	if (flag && !isDead_) {
		defeatEffectRequested_ = true;
		respawnTimer_ = 0.0f;
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

void Enemy::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	BaseCharacter::OnMapCollision(collisionMapinfo);

	if (collisionMapinfo.isHitWall_) {
		if (state_ == EnemyState::kPatrol) {
			velocity_.x *= -1.0f;
			lrDirection_ =
				velocity_.x > 0.0f ? LRDirection::kRight : LRDirection::kLeft;
		} else {
			velocity_.x = 0.0f;
		}
	}
}
