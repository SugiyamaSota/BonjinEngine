#include "BaseCharacter.h"
#include "../mapchip/MapChipField.h"
#include "../logic/Collision.h"

void BaseCharacter::UpdatePhysicsAndMapCollision() {
	// 重力適用
	velocity_.y -= kGravityAcceleration;
	if (velocity_.y < -kLimitFallSpeed) {
		velocity_.y = -kLimitFallSpeed;
	}

	if (mapChipField_) {
		// 1. マップとの当たり判定
		const CollisionMapInfo collisionMapinfo =
			ResolveMapCollision(*mapChipField_, worldTransform_.translate, velocity_, width_, height_);

		// 2. 移動反映
		worldTransform_.translate = Add(collisionMapinfo.movement_, worldTransform_.translate);

		// 3. 衝突結果に応じたステート適用
		OnMapCollision(collisionMapinfo);
	} else {
		worldTransform_.translate = Add(velocity_, worldTransform_.translate);
	}
}

void BaseCharacter::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	// 天井に当たっている場合
	if (collisionMapinfo.isHotTop_) {
		velocity_.y = 0.0f;
	}

	// 接地判定
	if (collisionMapinfo.isLandin_) {
		onGround_ = true;
		velocity_.y = 0.0f;
	} else if (velocity_.y > 0.0f ||
		!IsGroundedOnMap(*mapChipField_, worldTransform_.translate, width_, height_)) {
		onGround_ = false;
	}
}
