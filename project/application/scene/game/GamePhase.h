#pragma once

#include "../../engine/interface/BaseScene.h"
#include <vector>

namespace Bonjin {

class GameScene;

/// <summary>
/// GameSceneのフェーズ基底インターフェース
/// </summary>
using IGamePhase = IScenePhase<GameScene>;

/// <summary>
/// ハイライト演出フェーズ（カメラパン、実体化演出など）
/// </summary>
class GamePhaseHighlight : public IGamePhase {
public:
	enum class SubState {
		kFadeIn,
		kCameraPan,
		kFadeOut,
		kReturnToPlayer,
		kPlayerFocusIn
	};

	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void DrawImGui(GameScene* scene) override;
	void Exit(GameScene* scene) override;

	void FinishHighlight(GameScene* scene);

private:
	SubState subState_ = SubState::kFadeIn;
	size_t currentHighlightIndex_ = 0;
	float timer_ = 0.0f;
	Vector3 returnStartPos_ = { 0.0f, 0.0f, 0.0f };
	static constexpr float kFadeDuration = 0.5f;
	static constexpr float kPlayerFocusDuration = 0.8f;
	static constexpr float kReturnDuration = 1.8f;
	static constexpr float kMaxPullBackRadius = 90.0f;
};

/// <summary>
/// 通常プレイフェーズ（プレイヤー操作、敵挙動、ゴール/ゲームオーバー判定）
/// </summary>
class GamePhasePlay : public IGamePhase {
public:
	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void Exit(GameScene* scene) override;
};

/// <summary>
/// ゴール/ゲームオーバー後の遷移待ちフェーズ
/// </summary>
class GamePhaseGoal : public IGamePhase {
public:
	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void Exit(GameScene* scene) override;
};

}
