#include "NoGravityEnemy.h"
#include "ImGuiManager.h"
#include "../../logic/Collision.h"
#include "../../mapchip/MapChipField.h"
#include "../player/Player.h"
#include "ParticleManager.h"
#include <cmath>
#include <numbers>
#include <random>
#include <list>
#include <algorithm>

namespace Bonjin {

void NoGravityEnemy::Initialize(Object3D* model, Camera* camera, const Vector3& position) {
	GameObjectInitConfig config;
	config.physicsType = PhysicsType::Linear; // 重力なし
	config.hasCollider = true;
	config.categoryAttr = kAttributeEnemy;
	config.collisionMask = kAttributePlayer | kAttributeAnchor;
	config.tag = "Enemy";

	BaseEnemy::Initialize(model, camera, position, config);

	state_ = NoGravityEnemyState::kPatrol;
	floatingTimer_ = 0.0f;
	expReward_ = 40; // 空中敵は少し経験値を多くする

	chaseAngle_ = 0.0f;
	orbitDirection_ = 1.0f;
	isChaseInitialized_ = false;
	orbitRadius_ = 4.0f;
	orbitSpeed_ = 1.2f;
	minOrbitAngle_ = 0.0f;
	maxOrbitAngle_ = std::numbers::pi_v<float>;
	patrolBaseY_ = position.y;
}

void NoGravityEnemy::Update() {
	if (player_) {
		UpdateStateTransition();
	}

	floatingTimer_ += 1.0f / 60.0f;

	switch (state_) {
	case NoGravityEnemyState::kPatrol:
		PatrolBehavior();
		break;
	case NoGravityEnemyState::kChase:
		ChaseBehavior();
		break;
	}

	// 共通の更新処理（チャージや弾の更新）を呼ぶ
	BaseEnemy::Update();
	GameObject::Update();

	// 旋回制御
	TurningControl();

	// 浮遊アニメーション
	float param = std::sin(kPi * 2 * floatingTimer_ / 1.5f);
	worldTransform_.rotate.z = param * 0.1f; // 緩やかに傾く
}

bool NoGravityEnemy::IsPlayerVisible() const {
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

void NoGravityEnemy::UpdateStateTransition() {
	Vector3 playerPos = player_->GetWorldPosition();
	Vector3 myPos = GetWorldPosition();
	
	float distance = Length(Subtract(playerPos, myPos));

	if (state_ == NoGravityEnemyState::kPatrol) {
		if (distance <= searchRadius_ && IsPlayerVisible()) {
			chasingDirection_ = lrDirection_;
			state_ = NoGravityEnemyState::kChase;
			isChaseInitialized_ = false; // 追跡開始時に角度の初期設定を促す
		}
	} else if (state_ == NoGravityEnemyState::kChase) {
		if (distance > loseRadius_ || !IsPlayerVisible()) {
			state_ = NoGravityEnemyState::kPatrol;
			isChaseInitialized_ = false;
			patrolBaseY_ = worldTransform_.translate.y;
			lrDirection_ = chasingDirection_;
			if (lrDirection_ == LRDirection::kRight) {
				velocity_.x = kFlySpeed;
			} else {
				velocity_.x = -kFlySpeed;
			}
			velocity_.y = 0.0f;
		}
	}
}

void NoGravityEnemy::PatrolBehavior() {
	// 左右に移動
	if (lrDirection_ == LRDirection::kRight) {
		velocity_.x = kFlySpeed;
	} else {
		velocity_.x = -kFlySpeed;
	}
	velocity_.y = 0.0f;

	// サイン波でY座標をふわふわさせる
	float offset = std::sin(floatingTimer_ * kFloatSpeed) * kFloatAmplitude;
	worldTransform_.translate.y = patrolBaseY_ + offset;
}

void NoGravityEnemy::ChaseBehavior() {
	Vector3 playerPos = player_->GetWorldPosition();
	Vector3 myPos = GetWorldPosition();

	// 1. 初回の角度設定
	if (!isChaseInitialized_) {
		Vector3 diff = Subtract(myPos, playerPos);
		chaseAngle_ = std::atan2(diff.y, diff.x);
		// 角度を 0 ~ pi にクランプ
		chaseAngle_ = std::clamp(chaseAngle_, minOrbitAngle_, maxOrbitAngle_);
		isChaseInitialized_ = true;
	}

	// 2. 角度の更新
	chaseAngle_ += orbitDirection_ * orbitSpeed_ * (1.0f / 60.0f);

	// 3. 限界角度に達したら反転
	if (chaseAngle_ >= maxOrbitAngle_) {
		chaseAngle_ = maxOrbitAngle_;
		orbitDirection_ = -1.0f;
	} else if (chaseAngle_ <= minOrbitAngle_) {
		chaseAngle_ = minOrbitAngle_;
		orbitDirection_ = 1.0f;
	}

	// 4. 目標座標の計算 (プレイヤーを中心とする円運動)
	Vector3 targetPos;
	targetPos.x = playerPos.x + orbitRadius_ * std::cos(chaseAngle_);
	targetPos.y = playerPos.y + orbitRadius_ * std::sin(chaseAngle_);
	targetPos.z = spawnPosition_.z;

	// 5. 速度ベクトルの設定
	Vector3 toTarget = Subtract(targetPos, myPos);
	float distToTarget = Length(toTarget);
	if (distToTarget > 0.05f) {
		velocity_ = Multiply(kFlySpeed * 1.5f, Normalize(toTarget));
	} else {
		velocity_ = Vector3(0.0f, 0.0f, 0.0f);
	}

	// 向きの調整 (プレイヤーの位置を見て左右を向く)
	if (playerPos.x < myPos.x) {
		lrDirection_ = LRDirection::kLeft;
	} else {
		lrDirection_ = LRDirection::kRight;
	}

	// 追跡中、一定間隔でチャージを開始
	if (!isCharging_) {
		shootTimer_ -= 1.0f / 60.0f; // 60fps想定
		if (shootTimer_ <= 0.0f) {
			isCharging_ = true;
			chargeTimer_ = kChargeTime;

			// チャージ用のパーティクルをエミット
			uvScrollTimer_ = 0.0f;
			ParticleConfig config;
			config.position = myPos;
			config.scale = { kMaxChargeRingScale, kMaxChargeRingScale, 1.0f };
			config.rotate = { 0.0f, 0.0f, 0.0f };
			config.color = { 0.0f, 0.5f, 1.0f, 0.5f }; // 青色の半透明チャージリング
			config.lifeTime = kChargeTime;

			config.updateFunc = [this](ParticleData& data, float deltaTime) {
				float ratio = (data.lifeTime - data.currentTime) / data.lifeTime;
				ratio = (ratio < 0.0f) ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);

				float currentScale = ratio * kMaxChargeRingScale;
				data.transform.scale = { currentScale, currentScale, 1.0f };

				Vector3 pos = this->GetWorldPosition();
				data.transform.translate = { pos.x, pos.y, pos.z - 0.05f };
			};

			ParticleManager::GetInstance()->Emit("enemyChargeRing", config);
		}
	}
}

void NoGravityEnemy::TurningControl() {
	{
		float destinationRotationYTable[] = {
			std::numbers::pi_v<float> *-4.0f / 2.0f,
			std::numbers::pi_v<float> *2.0f / 2.0f,
		};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotate.y = destinationRotationY;
	}
}

void NoGravityEnemy::OnCollision() {
	SetIsLockedOn(true);
}

void NoGravityEnemy::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	GameObject::OnMapCollision(collisionMapinfo);

	// 壁に当たったら反転
	if (collisionMapinfo.isHitWall_ || collisionMapinfo.isHotTop_ || collisionMapinfo.isLandin_) {
		if (state_ == NoGravityEnemyState::kPatrol) {
			lrDirection_ = (lrDirection_ == LRDirection::kRight) ? LRDirection::kLeft : LRDirection::kRight;
		} else if (state_ == NoGravityEnemyState::kChase) {
			// 追従中は円運動の回転方向を反転
			orbitDirection_ *= -1.0f;
		}
	}
}

void NoGravityEnemy::DrawImGui(int index) {
#ifdef USE_IMGUI
	ImGui::PushID(index);
	std::string nodeName = "NoGravity Enemy " + std::to_string(index);
	if (isDead_) {
		nodeName += " [DEAD]";
	} else {
		nodeName += (state_ == NoGravityEnemyState::kPatrol) ? " [Patrol]" : " [CHASE]";
	}

	if (ImGui::TreeNode(nodeName.c_str())) {
		if (!isDead_) {
			Vector3 pos = GetWorldPosition();
			ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
			
			if (player_) {
				Vector3 pPos = player_->GetWorldPosition();
				float dist = Length(Subtract(pPos, pos));
				ImGui::Text("Distance to Player: %.2f", dist);
			}

			float sRad = searchRadius_;
			if (ImGui::SliderFloat("Search Radius", &sRad, 1.0f, 30.0f, "%.1f")) {
				searchRadius_ = sRad;
			}

			float lRad = loseRadius_;
			if (ImGui::SliderFloat("Lose Radius", &lRad, 1.0f, 30.0f, "%.1f")) {
				loseRadius_ = lRad;
			}

			// 円運動パラメータのImGui調整
			ImGui::SliderFloat("Orbit Radius", &orbitRadius_, 1.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Orbit Speed", &orbitSpeed_, 0.1f, 5.0f, "%.1f");

			int expRew = expReward_;
			if (ImGui::SliderInt("Exp Reward", &expRew, 0, 500)) {
				expReward_ = expRew;
			}
		} else {
			ImGui::Text("Dead...");
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
#endif
}

} // namespace Bonjin
