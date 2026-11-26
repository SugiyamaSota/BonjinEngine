#include "BaseEnemy.h"
#include "../player/Player.h"
#include "../../mapchip/MapChipField.h"
#include "ParticleManager.h"
#include "../../logic/Collision.h"
#include "Struct.h"
#include <cmath>
#include <numbers>

namespace Bonjin {

void BaseEnemy::Update() {
	if (player_) {
		UpdateStateTransition();
	}

	// 状態ごとの行動 (中身は敵の種類ごとに違う)
	switch (state_) {
	case State::kPatrol:
		PatrolBehavior();
		break;
	case State::kChase:
		ChaseBehavior();
		break;
	}

	UpdateChargeAndBullets();
	GameObject::Update();
	TurningControl();
	UpdateMotion();
}

void BaseEnemy::UpdateStateTransition() {
	const float distance = Length(Subtract(player_->GetWorldPosition(), GetWorldPosition()));

	if (state_ == State::kPatrol) {
		if (distance <= searchRadius_ && IsPlayerVisible()) {
			chasingDirection_ = lrDirection_;
			state_ = State::kChase;
			OnStartChase();
		}
	} else if (state_ == State::kChase) {
		if (distance > loseRadius_ || !IsPlayerVisible()) {
			state_ = State::kPatrol;
			lrDirection_ = chasingDirection_;
			OnLoseChase();
		}
	}
}

bool BaseEnemy::IsPlayerVisible() const {
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

void BaseEnemy::TurningControl() {
	const float destinationRotationYTable[] = {
		std::numbers::pi_v<float> * -4.0f / 2.0f,
		std::numbers::pi_v<float> * 2.0f / 2.0f,
	};
	worldTransform_.rotate.y = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
}

void BaseEnemy::UpdateChaseAttack() {
	const Vector3 playerPos = player_->GetWorldPosition();
	const Vector3 myPos = GetWorldPosition();

	// プレイヤーの方を向く
	lrDirection_ = (playerPos.x < myPos.x) ? LRDirection::kLeft : LRDirection::kRight;

	// 一定間隔でチャージを開始
	if (isCharging_) {
		return;
	}
	shootTimer_ -= 1.0f / 60.0f; // 60fps想定
	if (shootTimer_ > 0.0f) {
		return;
	}
	isCharging_ = true;
	chargeTimer_ = kChargeTime;

	// チャージ用のパーティクルをエミット
	uvScrollTimer_ = 0.0f;
	ParticleConfig config;
	config.position = myPos;
	config.scale = { kMaxChargeRingScale, kMaxChargeRingScale, 1.0f };
	config.rotate = { 0.0f, 0.0f, 0.0f };
	config.color = GetChargeRingColor();
	config.lifeTime = kChargeTime;

	// 毎フレーム敵の現在位置に追従しながら収縮する
	config.updateFunc = [this](ParticleData& data, float) {
		float ratio = (data.lifeTime - data.currentTime) / data.lifeTime;
		ratio = (ratio < 0.0f) ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);

		float currentScale = ratio * kMaxChargeRingScale;
		data.transform.scale = { currentScale, currentScale, 1.0f };

		Vector3 pos = this->GetWorldPosition();
		data.transform.translate = { pos.x, pos.y, pos.z - 0.05f };
	};

	ParticleManager::GetInstance()->Emit("enemyChargeRing", config);
}

std::string BaseEnemy::MakeImGuiNodeName(const char* name, int index) const {
	std::string nodeName = std::string(name) + " " + std::to_string(index);
	if (isDead_) {
		nodeName += " [DEAD]";
	} else {
		nodeName += (state_ == State::kPatrol) ? " [Patrol]" : " [CHASE]";
	}
	return nodeName;
}

void BaseEnemy::UpdateChargeAndBullets() {
	// チャージ更新
	if (isCharging_ && player_) {
		chargeTimer_ -= 1.0f / 60.0f; // 60fps想定

		// UVスクロール・スケールアニメーションの更新
		uvScrollTimer_ += 1.0f / 60.0f;
		Vector3 uvScale = Vector3(10.0f, 1.0f, 1.0f); // U方向に10倍スケール
		Vector3 uvTranslate = Vector3(-uvScrollTimer_ * 1.5f, 0.0f, 0.0f); // U方向スクロール
		Matrix4x4 uvTransform = MakeAffineMatrix(uvScale, Vector3(0.0f, 0.0f, 0.0f), uvTranslate);
		ParticleManager::GetInstance()->SetUVTransform("enemyChargeRing", uvTransform);

		if (chargeTimer_ <= 0.0f) {
			isCharging_ = false;
			Vector3 playerPos = player_->GetWorldPosition();
			Vector3 myPos = GetWorldPosition();
			Vector3 direction = Subtract(playerPos, myPos);
			if (Length(direction) > 0.05f) {
				Vector3 bulletVelocity = Multiply(kBulletSpeed, Normalize(direction));
				bulletVelocity.z = 0.0f;
				bullets_.push_back(std::make_unique<EnemyBullet>(myPos, bulletVelocity, camera_, mapChipField_));
			}
			shootTimer_ = kShootInterval;
		}
	}

	// 弾の更新
	for (auto it = bullets_.begin(); it != bullets_.end();) {
		(*it)->Update();

		if ((*it)->IsDead()) {
			it = bullets_.erase(it);
		} else {
			++it;
		}
	}
}

void BaseEnemy::Initialize(Object3D* model, Camera* camera, const Vector3& position, const GameObjectInitConfig& config) {
	spawnPosition_ = position;

	walkTimer_ = 0.0f;
	isLockedOn_ = false;
	isDead_ = false;
	defeatEffectRequested_ = false;
	respawnTimer_ = 0.0f;
	player_ = nullptr;
	lrDirection_ = LRDirection::kLeft;

	width_ = 2.0f;
	height_ = 2.0f;

	GameObject::Initialize(model, camera, position, config);

	velocity_ = Vector3(-0.05f, 0.0f, 0.0f);

	shootTimer_ = 0.0f;
	bullets_.clear();

	isCharging_ = false;
	chargeTimer_ = 0.0f;
	uvScrollTimer_ = 0.0f;
	expReward_ = 30;
	state_ = State::kPatrol;

	if (model_) {
		model_->SetColor(GetBaseColor());
	}

	searchRangeCircle_ = std::make_unique<DebugCircle3D>();
	searchRangeCircle_->Initialize();
}

void BaseEnemy::Draw() const {
	if (model_ && !isDead_) {
		model_->Draw();

		// デバッグ用索敵範囲の円を描画
		if (searchRangeCircle_ && camera_) {
			float radius = IsChasing() ? loseRadius_ : searchRadius_;
			Vector4 color = IsChasing() ? Vector4{ 1.0f, 0.5f, 0.0f, 1.0f } : Vector4{ 1.0f, 0.0f, 0.0f, 1.0f };
			searchRangeCircle_->Update(GetWorldPosition(), radius, camera_, color);
			searchRangeCircle_->Draw();
		}
	}
	for (auto& bullet : bullets_) {
		bullet->Draw();
	}
}

void BaseEnemy::UpdateRespawn(float deltaTime, float respawnTime, bool isEnemyRespawnEnabled) {
	if (!isDead_) {
		return;
	}

	if (!isEnemyRespawnEnabled) {
		respawnTimer_ = 0.0f;
		return;
	}

	respawnTimer_ += deltaTime;
	if (respawnTimer_ < respawnTime) {
		return;
	}

	worldTransform_ = {
		Vector3(1.0f, 1.0f, 1.0f),
		Vector3(0.0f, 0.0f, 0.0f),
		spawnPosition_,
	};
	lrDirection_ = LRDirection::kLeft;
	walkTimer_ = 0.0f;
	respawnTimer_ = 0.0f;
	isDead_ = false;
	isLockedOn_ = false;
	defeatEffectRequested_ = false;
	shootTimer_ = 0.0f;
	bullets_.clear();
	isCharging_ = false;
	chargeTimer_ = 0.0f;
	uvScrollTimer_ = 0.0f;

	GameObjectInitConfig config;
	config.physicsType = physicsType_;
	config.hasCollider = true;
	config.categoryAttr = kAttributeEnemy;
	config.collisionMask = kAttributePlayer | kAttributeAnchor;
	config.tag = "Enemy";
	GameObject::Initialize(model_, camera_, spawnPosition_, config);

	velocity_ = Vector3(-0.05f, 0.0f, 0.0f);

	model_->SetColor(GetBaseColor());
	model_->Update(worldTransform_, camera_);
}

void BaseEnemy::SetIsLockedOn(bool flag) {
	isLockedOn_ = flag;
	if (flag) {
		model_->SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	} else {
		model_->SetColor(GetBaseColor());
	}
}

void BaseEnemy::SetIsDead(bool flag) {
	if (flag && !isDead_) {
		defeatEffectRequested_ = true;
		respawnTimer_ = 0.0f;
		collider_.reset();
	}
	isDead_ = flag;
}

bool BaseEnemy::ConsumeDefeatEffectRequest() {
	if (!defeatEffectRequested_) {
		return false;
	}

	defeatEffectRequested_ = false;
	return true;
}

} // namespace Bonjin
