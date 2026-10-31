#pragma once

#include "IGamePhase.h"

namespace Bonjin::GamePhase {

/// <summary>
/// ゴール/ゲームオーバー後の遷移待ちフェーズ
/// </summary>
class Goal : public IGamePhase {
public:
	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void Exit(GameScene* scene) override;
};

}
