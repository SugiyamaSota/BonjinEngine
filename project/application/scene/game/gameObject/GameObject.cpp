#include "GameObject.h"
#include "../mapchip/MapChipField.h"
#include "../logic/Collision.h"
#include <cassert>

using namespace Bonjin;

void GameObject::Initialize(Object3D* model, Camera* camera, const Vector3& position, const GameObjectInitConfig& config) {
	model_ = model;
	camera_ = camera;

	worldTransform_ = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		position,
	};

	velocity_ = {};
	onGround_ = false;
	isDead_ = false;
	physicsType_ = config.physicsType;

	if (config.hasCollider) {
		collider_ = std::make_unique<Collider>(
			config.categoryAttr,
			config.collisionMask,
			this,
			config.tag
		);
		collider_->SetCallback([this](Collider* other) {
			OnCollision(other);
		});
	}
}

void GameObject::Update() {
	UpdatePhysicsAndMapCollision();

	if (collider_) {
		collider_->SetAABB(GetAABB());
	}

	if (model_) {
		model_->Update(worldTransform_, camera_);
	}
}

void GameObject::Draw() const {
	if (model_) {
		model_->Draw();
	}
}

AABB GameObject::GetAABB() const {
	AABB aabb;
	Vector3 worldPos = worldTransform_.translate;
	aabb.min = { worldPos.x - width_ / 2.0f, worldPos.y - height_ / 2.0f, worldPos.z - width_ / 2.0f };
	aabb.max = { worldPos.x + width_ / 2.0f, worldPos.y + height_ / 2.0f, worldPos.z + width_ / 2.0f };
	return aabb;
}

void GameObject::UpdatePhysicsAndMapCollision() {
	if (physicsType_ == PhysicsType::None) {
		return;
	}

	if (physicsType_ == PhysicsType::Gravity) {
		// 重力適用
		velocity_.y -= kGravityAcceleration;
		if (velocity_.y < -kLimitFallSpeed) {
			velocity_.y = -kLimitFallSpeed;
		}
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

void GameObject::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	if (physicsType_ == PhysicsType::Gravity) {
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
}
