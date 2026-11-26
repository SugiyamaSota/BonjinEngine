#include "DemoPlayerController.h"
#include "DemoManager.h"

#include "battle/BattleController.h"

#include <algorithm>
#include <cmath>

namespace Bonjin {

void DemoPlayerController::Update(const BattleController& battleController, float deltaTime, PlayerCommandList& commands) {
	// デモモードをOFFにしたら人の操作に戻す
	if (!DemoManager::GetInstance()->IsDemoMode()) {
		human_.Update(battleController, deltaTime, commands);
		return;
	}
	const Player* player = battleController.GetPlayer();
	if (!player) {
		return;
	}

	// 目の前に敵がいたら立ち止まって攻撃する
	const bool isAttacking = UpdateAttack(battleController, deltaTime, commands);

	const Vector3 playerPosition = player->GetPosition();
	const Vector3 goalPosition = battleController.GetGoalPosition();
	const float dx = goalPosition.x - playerPosition.x;
	const float dy = goalPosition.y - playerPosition.y;

	// ゴールの方へ歩く
	const bool moveRight = !isAttacking && dx > settings_.arriveDistance;
	const bool moveLeft = !isAttacking && dx < -settings_.arriveDistance;
	if (moveRight) {
		commands.push_back(std::make_unique<MoveCommand>(LRDirection::kRight));
	}
	if (moveLeft) {
		commands.push_back(std::make_unique<MoveCommand>(LRDirection::kLeft));
	}

	// 歩こうとしているのに進めていなければ詰まっている
	const bool isMoving = moveRight || moveLeft;
	if (isMoving && player->IsOnGround() && std::fabs(player->GetVelocity().x) < settings_.stuckSpeed) {
		stuckTimer_ += deltaTime;
	} else {
		stuckTimer_ = 0.0f;
	}

	// 詰まったとき・ゴールが上の近くにあるときはジャンプする
	jumpCooldownTimer_ -= deltaTime;
	const bool isGoalAboveAndNear = dy > settings_.jumpHeightThreshold && std::fabs(dx) < settings_.jumpNearDistance;
	const bool shouldJump = !isAttacking && player->IsOnGround() && jumpCooldownTimer_ <= 0.0f && jumpHoldTimer_ <= 0.0f &&
		(stuckTimer_ >= settings_.stuckTime || isGoalAboveAndNear);
	if (shouldJump) {
		commands.push_back(std::make_unique<JumpCommand>());
		jumpHoldTimer_ = settings_.jumpHoldTime;
		jumpCooldownTimer_ = settings_.jumpHoldTime + settings_.jumpCooldown;
		stuckTimer_ = 0.0f;
	}
	if (jumpHoldTimer_ > 0.0f) {
		jumpHoldTimer_ -= deltaTime;
	}
}

bool DemoPlayerController::UpdateAttack(const BattleController& battleController, float deltaTime, PlayerCommandList& commands) {
	const Player* player = battleController.GetPlayer();

	shootCooldownTimer_ = (std::max)(shootCooldownTimer_ - deltaTime, 0.0f);

	const bool hasLockedOnEnemy = player->GetLockedOnEnemiesCount() > 0;
	lockOnTimer_ = hasLockedOnEnemy ? lockOnTimer_ + deltaTime : 0.0f;

	const bool hasTarget = FindEnemyInFront(battleController) != nullptr;
	const bool canShoot = hasTarget && !player->HasAnchor() && player->GetAnchorStock() > 0;

	bool isHoldingAnchor = false; // このフレームにアンカーのボタンを押すか
	bool shouldStop = false;
	if (canShoot) {
		// まだロックオンしていない敵が目の前にいればアンカーを撃つ (何体かいれば先に全員ロックオンする)
		if (shootCooldownTimer_ <= 0.0f) {
			isHoldingAnchor = true;
			shootCooldownTimer_ = settings_.shootInterval;
		}
		shouldStop = true;
	} else if (hasLockedOnEnemy) {
		// ロックオンした敵を少し見せてから撃破する
		if (lockOnTimer_ >= settings_.defeatDelay) {
			commands.push_back(std::make_unique<DefeatLockedOnCommand>());
			lockOnTimer_ = 0.0f;
		}
		shouldStop = true;
	} else if (player->HasAnchor() && player->GetAnchor().GetStandBy() && !player->GetAnchor().IsDead()) {
		// 外れて壁に刺さったアンカーが残っていると次が撃てないので、長押しで回収する
		isHoldingAnchor = true;
		shouldStop = hasTarget;
	}

	// アンカーのボタンは、人の操作と同じく「押した瞬間 (発射)」と「押し続け (回収)」の命令で出す
	if (isHoldingAnchor) {
		if (!wasAnchorHeld_) {
			commands.push_back(std::make_unique<TriggerAnchorCommand>());
		}
		commands.push_back(std::make_unique<HoldAnchorCommand>());
	}
	wasAnchorHeld_ = isHoldingAnchor;
	return shouldStop;
}

const BaseEnemy* DemoPlayerController::FindEnemyInFront(const BattleController& battleController) const {
	const Player* player = battleController.GetPlayer();
	const MapChipField* mapChipField = battleController.GetMapChipField();
	if (!mapChipField) {
		return nullptr;
	}
	const Vector3 playerPosition = player->GetPosition();
	const float direction = player->IsFacingRight() ? 1.0f : -1.0f;
	constexpr float kAnchorHalfHeight = 0.5f; // アンカーの当たり判定の高さの半分
	constexpr float kWallCheckStep = 0.5f;

	const BaseEnemy* nearestEnemy = nullptr;
	float nearestDistance = settings_.attackRange;
	for (const auto& enemy : battleController.GetEnemies()) {
		if (enemy->GetIsDead() || enemy->GetIsLockedOn()) {
			continue;
		}
		// 向いている側の近くにいるか
		const float distance = (enemy->GetWorldPosition().x - playerPosition.x) * direction;
		if (distance <= 0.0f || distance > nearestDistance) {
			continue;
		}
		// まっすぐ撃ったアンカーが当たる高さか
		const AABB enemyAABB = enemy->GetAABB();
		const float overlap =
			(std::min)(playerPosition.y + kAnchorHalfHeight, enemyAABB.max.y) -
			(std::max)(playerPosition.y - kAnchorHalfHeight, enemyAABB.min.y);
		if (overlap < settings_.attackHeightMargin) {
			continue;
		}
		// 間に壁がないか
		bool isBlocked = false;
		for (float d = kWallCheckStep; d < distance; d += kWallCheckStep) {
			const IndexSet index = mapChipField->GetMapChipIndexSetByPosition(
				{ playerPosition.x + direction * d, playerPosition.y, playerPosition.z });
			if (mapChipField->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipType::kBlock) {
				isBlocked = true;
				break;
			}
		}
		if (isBlocked) {
			continue;
		}
		nearestEnemy = enemy.get();
		nearestDistance = distance;
	}
	return nearestEnemy;
}

}
