#pragma once
#include <cstdint>

namespace Bonjin {

enum CollisionAttribute : uint32_t {
	kAttributeNone   = 0,
	kAttributePlayer = 1 << 0,
	kAttributeEnemy  = 1 << 1,
	kAttributeAnchor = 1 << 2,
	kAttributeGoal   = 1 << 3,
	kAttributeEnemyBullet = 1 << 4,
};

}
