#pragma once

#include "IGamePhase.h"

namespace Bonjin::GamePhase {

/// <summary>
/// 通常プレイフェーズ（プレイヤー操作、敵挙動、ゴール/ゲームオーバー判定）
/// </summary>
class Play : public IGamePhase {
public:
	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void Exit(GameScene* scene) override;
};

}
