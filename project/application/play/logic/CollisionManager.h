#pragma once
#include <vector>

namespace Bonjin {

class Collider;

class CollisionManager {
public:
	static CollisionManager* GetInstance();

private:
	CollisionManager() = default;
	~CollisionManager() = default;
	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator=(const CollisionManager&) = delete;

	std::vector<Collider*> colliders_;

public:
	void RegisterCollider(Collider* collider);
	void UnregisterCollider(Collider* collider);
	void Clear();
	void CheckAllCollisions();
};

}
