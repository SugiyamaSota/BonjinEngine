#include "CollisionManager.h"
#include "Collider.h"
#include "Collision.h"
#include <algorithm>

using namespace Bonjin;

// Colliderの実装
Collider::Collider(uint32_t categoryAttr, uint32_t collisionMask, void* owner, const std::string& tag)
	: categoryAttr_(categoryAttr), collisionMask_(collisionMask), owner_(owner), tag_(tag) {
	CollisionManager::GetInstance()->RegisterCollider(this);
}

Collider::~Collider() {
	CollisionManager::GetInstance()->UnregisterCollider(this);
}

// CollisionManagerの実装
CollisionManager* CollisionManager::GetInstance() {
	static CollisionManager instance;
	return &instance;
}

void CollisionManager::RegisterCollider(Collider* collider) {
	colliders_.push_back(collider);
}

void CollisionManager::UnregisterCollider(Collider* collider) {
	auto it = std::find(colliders_.begin(), colliders_.end(), collider);
	if (it != colliders_.end()) {
		colliders_.erase(it);
	}
}

void CollisionManager::Clear() {
	colliders_.clear();
}

void CollisionManager::CheckAllCollisions() {
	for (size_t i = 0; i < colliders_.size(); ++i) {
		for (size_t j = i + 1; j < colliders_.size(); ++j) {
			Collider* cA = colliders_[i];
			Collider* cB = colliders_[j];

			// 衝突フィルターのチェック
			if (!(cA->GetCollisionMask() & cB->GetCategoryAttr()) ||
				!(cB->GetCollisionMask() & cA->GetCategoryAttr())) {
				continue;
			}

			// AABB交差判定
			if (IsCollision(cA->GetAABB(), cB->GetAABB())) {
				cA->OnCollision(cB);
				cB->OnCollision(cA);
			}
		}
	}
}
