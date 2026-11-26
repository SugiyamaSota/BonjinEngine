#pragma once
#include "BaseEnemy.h"
#include <memory>
#include <string>

namespace Bonjin {

/// <summary>
/// マップに書かれた敵の種類の名前から、敵を作る
/// 敵の種類を増やすときはここの表に1行足すだけでよい (呼ぶ側は BaseEnemy としてだけ扱う)
/// </summary>
class EnemyFactory {
public:
	/// <summary>
	/// 敵を作る (知らない名前なら地上の敵)
	/// </summary>
	static std::unique_ptr<BaseEnemy> Create(const std::string& type);
};

}
