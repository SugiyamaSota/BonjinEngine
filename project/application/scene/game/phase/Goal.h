#pragma once

#include "IGamePhase.h"

namespace Bonjin::GamePhase {

/// <summary>
/// ゴール後のクリア演出フェーズ
/// プレイヤーのFlair演出を再生し、終わったらリザルトシーンへ遷移する
/// </summary>
class Goal : public IGamePhase {
public:
	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void Exit(GameScene* scene) override;

private:
	bool isSceneChangeRequested_ = false;
	float elapsedTime_ = 0.0f;
	// 演出が終わらなくてもこの秒数で遷移する (アニメーションが読めなかった時などの保険)
	static inline const float kMaxPerformanceTime = 10.0f;
};

}
