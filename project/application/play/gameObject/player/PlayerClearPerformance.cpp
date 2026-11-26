#include "PlayerClearPerformance.h"
#include <algorithm>

using AnimState = PlayerAnimator::State;

namespace {

// 各段階は外から直接作らないので、このファイルの中だけに置く

/// <summary>
/// 演出終了 (最後のポーズのまま待つ)
/// </summary>
class DoneState : public IClearState {
public:
	PlayerClearPhase GetPhase() const override { return PlayerClearPhase::kDone; }
	std::unique_ptr<IClearState> Update(PlayerClearPerformance&) override { return nullptr; }
	AnimState GetAnimation(const PlayerClearPerformance&) const override { return AnimState::kFlairFinish; }
	float GetCameraZoom(const PlayerClearPerformance&) const override { return 1.0f; }
	float GetLightningBlend(float) const override { return 1.0f; }
};

/// <summary>
/// 決めポーズで少し溜める -> ふくらむ -> 一気に縮んで消える (転移した風)
/// </summary>
class VanishState : public IClearState {
public:
	PlayerClearPhase GetPhase() const override { return PlayerClearPhase::kVanish; }

	std::unique_ptr<IClearState> Update(PlayerClearPerformance& performance) override {
		const ClearPerformanceSettings& s = performance.GetSettings();
		const float growStart = s.poseHoldTime;
		const float shrinkStart = growStart + s.vanishGrowTime;
		const float end = shrinkStart + s.vanishShrinkTime;
		const float t = performance.GetTimer();
		if (t < growStart) {
			performance.SetVisualScale(1.0f);
		} else if (t < shrinkStart) {
			// ease out でふくらむ
			float r = s.vanishGrowTime > 0.0f ? (t - growStart) / s.vanishGrowTime : 1.0f;
			r = 1.0f - (1.0f - r) * (1.0f - r);
			performance.SetVisualScale(1.0f + (s.vanishGrowScale - 1.0f) * r);
		} else if (t < end) {
			// ease in で一気に縮む
			float r = s.vanishShrinkTime > 0.0f ? (t - shrinkStart) / s.vanishShrinkTime : 1.0f;
			r = r * r * r;
			performance.SetVisualScale(s.vanishGrowScale * (1.0f - r));
		} else {
			performance.SetVisualScale(0.0f);
			// 消えた瞬間に、テレポートと同じ雷を上へ走らせる
			if (s.vanishLightning) {
				performance.RequestVanishLightning();
			}
			return std::make_unique<DoneState>();
		}
		return nullptr;
	}

	AnimState GetAnimation(const PlayerClearPerformance&) const override { return AnimState::kFlairFinish; } // 最後のポーズのまま
	float GetCameraZoom(const PlayerClearPerformance&) const override { return 1.0f; }
	float GetLightningBlend(float) const override { return 1.0f; }
};

/// <summary>
/// Flair終了 (寄りの距離のまま、ポータルは雷の色)
/// </summary>
class FinishState : public IClearState {
public:
	PlayerClearPhase GetPhase() const override { return PlayerClearPhase::kFinish; }

	std::unique_ptr<IClearState> Update(PlayerClearPerformance& performance) override {
		if (performance.GetTimer() >= performance.GetInput().flairFinishLength) {
			return std::make_unique<VanishState>();
		}
		return nullptr;
	}

	AnimState GetAnimation(const PlayerClearPerformance&) const override { return AnimState::kFlairFinish; }
	float GetCameraZoom(const PlayerClearPerformance&) const override { return 1.0f; }
	float GetLightningBlend(float) const override { return 1.0f; } // 決めポーズから消えるまでは雷の色のまま
};

/// <summary>
/// Flairループ (カメラが一定間隔で寄り、寄るたびにポータルが雷の色へ近づく)
/// </summary>
class LoopState : public IClearState {
public:
	PlayerClearPhase GetPhase() const override { return PlayerClearPhase::kLoop; }

	std::unique_ptr<IClearState> Update(PlayerClearPerformance& performance) override {
		const float loopTime = performance.GetInput().flairingLength * static_cast<float>(performance.GetSettings().flairLoopCount);
		if (performance.GetTimer() >= loopTime) {
			return std::make_unique<FinishState>();
		}
		return nullptr;
	}

	AnimState GetAnimation(const PlayerClearPerformance&) const override { return AnimState::kFlairing; }
	float GetCameraZoom(const PlayerClearPerformance&) const override { return 1.0f; }
	float GetLightningBlend(float loopProgress) const override { return loopProgress; }
};

/// <summary>
/// Flair開始 (カメラが通常の距離から寄りの距離へ近づく)
/// </summary>
class StartState : public IClearState {
public:
	PlayerClearPhase GetPhase() const override { return PlayerClearPhase::kStart; }

	std::unique_ptr<IClearState> Update(PlayerClearPerformance& performance) override {
		if (performance.GetTimer() >= performance.GetInput().flairStartLength) {
			return std::make_unique<LoopState>();
		}
		return nullptr;
	}

	AnimState GetAnimation(const PlayerClearPerformance&) const override { return AnimState::kFlairStart; }

	float GetCameraZoom(const PlayerClearPerformance& performance) const override {
		// Flair開始の長さをかけて寄せる (ease out)
		const float length = performance.GetInput().flairStartLength;
		float t = length > 0.0f ? (std::min)(performance.GetTimer() / length, 1.0f) : 1.0f;
		return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	}

	float GetLightningBlend(float) const override { return 0.0f; }
};

/// <summary>
/// 着地待ち (空中ならジャンプのポーズで落ちるのを待つ)
/// </summary>
class LandingState : public IClearState {
public:
	PlayerClearPhase GetPhase() const override { return PlayerClearPhase::kLanding; }

	std::unique_ptr<IClearState> Update(PlayerClearPerformance& performance) override {
		if (performance.GetInput().isOnGround || performance.GetTimer() >= performance.GetSettings().landingTimeout) {
			return std::make_unique<StartState>();
		}
		return nullptr;
	}

	AnimState GetAnimation(const PlayerClearPerformance& performance) const override {
		return performance.GetInput().isOnGround ? AnimState::kIdle : AnimState::kJump;
	}

	float GetCameraZoom(const PlayerClearPerformance&) const override { return 0.0f; }
	float GetLightningBlend(float) const override { return 0.0f; }
};

} // namespace

PlayerClearPerformance::PlayerClearPerformance() = default;
PlayerClearPerformance::~PlayerClearPerformance() = default;

void PlayerClearPerformance::Start() {
	visualScale_ = 1.0f;
	vanishLightningRequested_ = false;
	ChangeState(std::make_unique<LandingState>());
}

void PlayerClearPerformance::Stop() {
	state_.reset();
	timer_ = 0.0f;
	visualScale_ = 1.0f;
	vanishLightningRequested_ = false;
}

void PlayerClearPerformance::Skip() {
	ChangeState(std::make_unique<DoneState>());
}

void PlayerClearPerformance::Update(const ClearPerformanceInput& input, float deltaTime) {
	if (!state_) {
		return;
	}
	input_ = input;
	timer_ += deltaTime;
	if (std::unique_ptr<IClearState> next = state_->Update(*this)) {
		ChangeState(std::move(next));
	}
}

PlayerClearPhase PlayerClearPerformance::GetPhase() const {
	return state_ ? state_->GetPhase() : PlayerClearPhase::kNone;
}

PlayerAnimator::State PlayerClearPerformance::GetAnimation() const {
	return state_ ? state_->GetAnimation(*this) : AnimState::kIdle;
}

float PlayerClearPerformance::GetCameraZoom() const {
	return state_ ? state_->GetCameraZoom(*this) : 0.0f;
}

float PlayerClearPerformance::GetLightningBlend(float loopProgress) const {
	return state_ ? state_->GetLightningBlend((std::clamp)(loopProgress, 0.0f, 1.0f)) : 0.0f;
}

bool PlayerClearPerformance::ConsumeVanishLightningRequest() {
	const bool requested = vanishLightningRequested_;
	vanishLightningRequested_ = false;
	return requested;
}

void PlayerClearPerformance::ChangeState(std::unique_ptr<IClearState> next) {
	state_ = std::move(next);
	timer_ = 0.0f;
}
