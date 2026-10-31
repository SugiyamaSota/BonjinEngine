#pragma once

#include "IGamePhase.h"
#include "PostEffectTypes.h"
#include "vector.h"

namespace Bonjin::GamePhase {

/// <summary>
/// ハイライト演出フェーズ（カメラパン、実体化演出など）
/// </summary>
class Highlight : public IGamePhase {
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
	void SetupHighlightDissolve();

	SubState subState_ = SubState::kFadeIn;
	size_t currentHighlightIndex_ = 0;
	float timer_ = 0.0f;
	Vector3 returnStartPos_ = { 0.0f, 0.0f, 0.0f };
	DissolveMode dissolveMode_ = DissolveMode::kRightToLeft;
	static constexpr float kFadeDuration = 0.5f;
	static constexpr float kPlayerFocusDuration = 0.8f;
	static constexpr float kReturnDuration = 1.8f;
	static constexpr float kMaxPullBackRadius = 90.0f;
};

}
