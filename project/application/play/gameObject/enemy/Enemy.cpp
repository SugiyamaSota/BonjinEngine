#include "Enemy.h"
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

void Enemy::Initialize(Object3D* model, Camera* camera, const Vector3& position) {
	GameObjectInitConfig config;
	config.physicsType = PhysicsType::Gravity;
	config.hasCollider = true;
	config.categoryAttr = kAttributeEnemy;
	config.collisionMask = kAttributePlayer | kAttributeAnchor;
	config.tag = "Enemy";

	BaseEnemy::Initialize(model, camera, position, config);

	state_ = EnemyState::kPatrol;
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

	// 共通の更新処理（チャージや弾の更新）を呼ぶ
	BaseEnemy::Update();
	GameObject::Update();

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
			config.color = { 1.0f, 0.0f, 0.0f, 0.5f }; // 赤色の半透明
			config.lifeTime = kChargeTime;

			// 毎フレーム敵の現在位置に追従しながら収縮する更新処理を設定
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

void Enemy::TurningControl() {
	{
		float destinationRotationYTable[] = {
			std::numbers::pi_v<float> *-4.0f / 2.0f,
			std::numbers::pi_v<float> *2.0f / 2.0f,
		};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotate.y = destinationRotationY;
	}
}

void Enemy::OnCollision() {
	SetIsLockedOn(true);
}

void Enemy::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	GameObject::OnMapCollision(collisionMapinfo);

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

void Enemy::DrawImGui(int index) {
#ifdef USE_IMGUI
	ImGui::PushID(index);
	std::string nodeName = "Walking Enemy " + std::to_string(index);
	if (isDead_) {
		nodeName += " [DEAD]";
	} else {
		nodeName += (state_ == EnemyState::kPatrol) ? " [Patrol]" : " [CHASE]";
	}

	if (ImGui::TreeNode(nodeName.c_str())) {
		if (!isDead_) {
			Vector3 pos = GetWorldPosition();
			ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
			ImGui::Text("On Ground: %s", IsOnGround() ? "true" : "false");
			
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

bool Enemy::IsChasing() const {
	return state_ == EnemyState::kChase;
}

} // namespace Bonjin
