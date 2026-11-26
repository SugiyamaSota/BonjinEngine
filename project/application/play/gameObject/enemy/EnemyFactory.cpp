#include "EnemyFactory.h"
#include "Enemy.h"
#include "NoGravityEnemy.h"
#include <functional>
#include <unordered_map>

namespace Bonjin {

std::unique_ptr<BaseEnemy> EnemyFactory::Create(const std::string& type) {
	using Creator = std::function<std::unique_ptr<BaseEnemy>()>;
	// 種類の名前 -> 作り方
	static const std::unordered_map<std::string, Creator> kCreators = {
		{ "enemy",          [] { return std::make_unique<Enemy>(); } },
		{ "nogravityenemy", [] { return std::make_unique<NoGravityEnemy>(); } },
		{ "E2",             [] { return std::make_unique<NoGravityEnemy>(); } },
	};

	if (auto it = kCreators.find(type); it != kCreators.end()) {
		return it->second();
	}
	return std::make_unique<Enemy>(); 
}

}
