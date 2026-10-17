#pragma once
#include <functional>
#include <string>
#include "CollisionConfig.h"
#include "Struct.h"

namespace Bonjin {

class Collider {
private:
	AABB aabb_{};
	uint32_t categoryAttr_ = 0;
	uint32_t collisionMask_ = 0;
	std::function<void(Collider*)> onCollisionCallback_;
	void* owner_ = nullptr;
	std::string tag_ = "";

public:
	Collider(uint32_t categoryAttr, uint32_t collisionMask, void* owner, const std::string& tag = "");
	~Collider();

	void SetAABB(const AABB& aabb) { aabb_ = aabb; }
	const AABB& GetAABB() const { return aabb_; }
	uint32_t GetCategoryAttr() const { return categoryAttr_; }
	uint32_t GetCollisionMask() const { return collisionMask_; }
	void* GetOwner() const { return owner_; }
	const std::string& GetTag() const { return tag_; }

	void SetCallback(std::function<void(Collider*)> callback) { onCollisionCallback_ = callback; }

	void OnCollision(Collider* other) {
		if (onCollisionCallback_) {
			onCollisionCallback_(other);
		}
	}
};

}
