#include "Highlight.h"
#include "Play.h"
#include "../GameScene.h"

#include "SceneManager.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "vector.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace Bonjin::GamePhase {

/// <summary>
/// ハイライト演出の段階を1フレーム進めた結果
/// </summary>
struct HighlightStepResult {
	std::unique_ptr<IHighlightStep> next; // 次の段階 (nullptr ならこのまま)
	bool isFinished = false;              // ハイライト演出を終えてプレイフェーズへ進む

	static HighlightStepResult Stay() { return {}; }
	static HighlightStepResult Next(std::unique_ptr<IHighlightStep> step) { return { std::move(step), false }; }
	static HighlightStepResult Finish() { return { nullptr, true }; }
};

/// <summary>
/// ハイライト演出の1段階 (State)
/// </summary>
class IHighlightStep {
public:
	virtual ~IHighlightStep() = default;

	// この段階に入ったとき
	virtual void Enter(BattleController& battleController) { (void)battleController; }

	/// <summary>
	/// 1フレーム進める
	/// </summary>
	/// <param name="timer">この段階に入ってからの経過時間</param>
	virtual HighlightStepResult Update(BattleController& battleController, float timer, float deltaTime) = 0;

	virtual void DrawImGui(const BattleController& battleController) const = 0;
};

namespace {

constexpr float kFadeDuration = 0.5f;
constexpr float kReturnDuration = 1.8f;
constexpr float kMaxPullBackRadius = 90.0f;

const std::vector<StageHighlightData>& GetHighlights(const BattleController& battleController) {
	return battleController.GetMapChipField()->GetHighlights();
}

Vector3 GetPlayerPosition(const BattleController& battleController) {
	return battleController.GetPlayer() ? battleController.GetPlayer()->GetWorldPosition() : Vector3{ 0.0f, 0.0f, 0.0f };
}

/// <summary>
/// 何番目のハイライトを見せているかを持つ段階の共通部分
/// </summary>
class IndexedHighlightStep : public IHighlightStep {
public:
	explicit IndexedHighlightStep(size_t index) : index_(index) {}

	void DrawImGui(const BattleController& battleController) const override {
#ifdef USE_IMGUI
		ImGui::Text("Highlight: %zu / %zu", index_ + 1, GetHighlights(battleController).size());
#else
		(void)battleController;
#endif
	}

protected:
	size_t index_ = 0;
};

/// <summary>
/// 最後のハイライトから、カメラを引きながらプレイヤーへ戻る
/// </summary>
class ReturnToPlayerStep : public IHighlightStep {
public:
	explicit ReturnToPlayerStep(const Vector3& startPos) : startPos_(startPos) {}

	void Enter(BattleController&) override {
		// 画面は明るい状態（FadeAmount = 1.0f）を維持
		auto sm = SceneManager::GetInstance();
		sm->ClearPostEffects();
		sm->SetFadeAmount(1.0f);
		sm->SetDissolveThreshold(1.0f);
	}

	HighlightStepResult Update(BattleController& battleController, float timer, float) override {
		CameraController* camera = battleController.GetCameraController();
		float t = std::clamp(timer / kReturnDuration, 0.0f, 1.0f);

		// なめらかな補間カーブ (SmootherStep: 6t^5 - 15t^4 + 10t^3)
		float smoothT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

		const Vector3 playerPos = GetPlayerPosition(battleController);
		Vector3 currentTarget = Lerp(startPos_, playerPos, smoothT);

		// カメラ注視点をプレイヤー位置へ向けて更新
		camera->SetTargetInstant(currentTarget);

		// 移動中にカメラを引き（ズームアウト）にし、プレイヤーに近づくにつれて通常距離に戻す
		float pullFactor = std::sin(t * 3.1415926535f);
		float currentRadius = Camera::kDefaultRadius + (kMaxPullBackRadius - Camera::kDefaultRadius) * pullFactor;
		camera->SetCameraRadius(currentRadius);

		if (timer >= kReturnDuration) {
			// プレイヤーに到着完了！カメラ距離を確実に通常に戻し、GOシグナルを発行
			camera->ResetCameraRadius();
			camera->SetTargetInstant(playerPos);
			return HighlightStepResult::Finish();
		}
		return HighlightStepResult::Stay();
	}

	void DrawImGui(const BattleController&) const override {
#ifdef USE_IMGUI
		ImGui::Text("State: Return to Player (Pull-back Camera)");
#endif
	}

private:
	Vector3 startPos_{};
};

class FadeInStep; // FadeOutStep から次のハイライトへ戻るため

/// <summary>
/// 次のハイライトへ向けて暗転していく
/// </summary>
class FadeOutStep : public IndexedHighlightStep {
public:
	using IndexedHighlightStep::IndexedHighlightStep;

	void Enter(BattleController&) override {
		auto sm = SceneManager::GetInstance();
		sm->ClearPostEffects();
		sm->AddPostEffect(PostEffect::kSimpleFade);
		sm->SetFadeAmount(1.0f);
	}

	HighlightStepResult Update(BattleController& battleController, float timer, float) override;
};

/// <summary>
/// ハイライトの場所をカメラでなぞる
/// </summary>
class CameraPanStep : public IndexedHighlightStep {
public:
	using IndexedHighlightStep::IndexedHighlightStep;

	void Enter(BattleController&) override {
		// パン中は通常描画（エフェクトなしで鮮明に）
		auto sm = SceneManager::GetInstance();
		sm->ClearPostEffects();
		sm->SetFadeAmount(1.0f);
	}

	HighlightStepResult Update(BattleController& battleController, float timer, float deltaTime) override {
		const auto& highlights = GetHighlights(battleController);
		if (index_ >= highlights.size()) {
			return HighlightStepResult::Next(std::make_unique<ReturnToPlayerStep>(GetPlayerPosition(battleController)));
		}

		const auto& currentHighlight = highlights[index_];
		float duration = (std::max)(currentHighlight.displayDuration, 0.1f);
		float t = std::clamp(timer / duration, 0.0f, 1.0f);

		// スムーズな補間 (3t^2 - 2t^3)
		float smoothT = t * t * (3.0f - 2.0f * t);
		Vector3 targetPos = Add(currentHighlight.startWorldPos, Multiply(smoothT, currentHighlight.panDirection));
		battleController.GetCameraController()->Update(targetPos, deltaTime);

		if (timer < duration) {
			return HighlightStepResult::Stay();
		}
		// 3つ目（または最後のハイライト）に達した場合はフェードアウトせずにカメラを引きながらプレイヤーへ移動
		const bool isLastHighlight = (index_ >= 2 || index_ + 1 >= highlights.size());
		if (isLastHighlight) {
			return HighlightStepResult::Next(std::make_unique<ReturnToPlayerStep>(targetPos));
		}
		return HighlightStepResult::Next(std::make_unique<FadeOutStep>(index_));
	}
};

/// <summary>
/// ハイライトの場所へカメラを移し、徐々に実体化する
/// </summary>
class FadeInStep : public IndexedHighlightStep {
public:
	using IndexedHighlightStep::IndexedHighlightStep;

	void Enter(BattleController& battleController) override {
		// ハイライトの場所へカメラを瞬時にワープし、描画用モデルも合わせて更新
		const auto& highlights = GetHighlights(battleController);
		if (index_ < highlights.size()) {
			battleController.GetCameraController()->SetTargetInstant(highlights[index_].startWorldPos);
		}
		battleController.UpdateVisuals(0.0f);
	}

	HighlightStepResult Update(BattleController&, float timer, float) override {
		float t = std::clamp(timer / kFadeDuration, 0.0f, 1.0f);
		SceneManager::GetInstance()->SetFadeAmount(t); // 徐々に実体化（0.0 -> 1.0）

		if (timer >= kFadeDuration) {
			return HighlightStepResult::Next(std::make_unique<CameraPanStep>(index_));
		}
		return HighlightStepResult::Stay();
	}
};

HighlightStepResult FadeOutStep::Update(BattleController& battleController, float timer, float) {
	float t = 1.0f - std::clamp(timer / kFadeDuration, 0.0f, 1.0f);
	SceneManager::GetInstance()->SetFadeAmount(t); // 徐々に暗くなる

	if (timer < kFadeDuration) {
		return HighlightStepResult::Stay();
	}
	const size_t nextIndex = index_ + 1;
	if (nextIndex < GetHighlights(battleController).size()) {
		// 次のハイライトへカメラを瞬時に移動させて Fade In
		return HighlightStepResult::Next(std::make_unique<FadeInStep>(nextIndex));
	}
	return HighlightStepResult::Next(std::make_unique<ReturnToPlayerStep>(GetPlayerPosition(battleController)));
}

} // namespace

Highlight::Highlight() = default;
Highlight::~Highlight() = default;

void Highlight::SetupHighlightDissolve() {
	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->AddPostEffect(PostEffect::kDigitalDissolve);
	sm->SetDissolveMode(dissolveMode_);
	sm->SetDissolveThreshold(0.0f); // 真っ黒から実体化スタート
	sm->SetDissolveEdgeWidth(0.06f); // サイバー発光エッジ
	sm->SetDissolveEdgeColor({ 0.0f, 1.5f, 2.0f }); // サイバーブルー
}

void Highlight::ChangeStep(std::unique_ptr<IHighlightStep> next, BattleController& battleController) {
	step_ = std::move(next);
	timer_ = 0.0f;
	if (step_) {
		step_->Enter(battleController);
	}
}

void Highlight::Enter(GameScene* scene) {
	timer_ = 0.0f;

	auto bc = scene->GetBattleController();
	if (!bc) {
		return;
	}

	// 演出中はHUDを非表示
	bc->SetHudVisible(false);

	// 最初のハイライトから始める (カメラを最初の位置へワープ)
	ChangeStep(std::make_unique<FadeInStep>(0), *bc);

	// 初回ハイライトのDigitalDissolveを開始
	SetupHighlightDissolve();
}

void Highlight::Update(GameScene* scene, float deltaTime) {
	auto bc = scene->GetBattleController();
	if (!bc || !step_) {
		return;
	}

	// スキップ判定 (SPACEキー または GAMEPAD Aボタン)
	// ※ シーン切り替え直後の入力残りや連打による誤スキップを防ぐため0.2秒のガードを設ける
	if (timer_ > 0.2f && (Input::GetInstance()->IsTrigger(DIK_SPACE) ||
		Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A))) {
		FinishHighlight(scene);
		return;
	}

	timer_ += deltaTime;

	// 今の段階を進める (段階ごとの処理は各 IHighlightStep のクラスが持つ)
	HighlightStepResult result = step_->Update(*bc, timer_, deltaTime);
	if (result.isFinished) {
		FinishHighlight(scene); // プレイフェーズへ切り替わり、このフェーズは破棄される
		return;
	}
	if (result.next) {
		ChangeStep(std::move(result.next), *bc);
	}

	// 描画用モデル（カメラ追従WVP行列、天球、ブロック、ゴール、パーティクル等）を更新
	bc->UpdateVisuals(deltaTime);
}

void Highlight::DrawImGui(GameScene* scene) {
#ifdef USE_IMGUI
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "=== Stage Intro Highlight ===");
	auto bc = scene->GetBattleController();
	if (bc && bc->GetMapChipField() && step_) {
		step_->DrawImGui(*bc);
	}
	if (ImGui::Button("Skip Highlight")) {
		FinishHighlight(scene);
	}
	ImGui::Separator();
#else
	(void)scene;
#endif
}

void Highlight::Exit(GameScene* scene) {
	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->SetFadeAmount(1.0f);
	sm->SetDissolveThreshold(1.0f);

	auto bc = scene->GetBattleController();
	if (bc && bc->GetCameraController()) {
		bc->GetCameraController()->ResetCameraRadius();
	}
}

void Highlight::FinishHighlight(GameScene* scene) {
	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->SetFadeAmount(1.0f);
	sm->SetDissolveThreshold(1.0f);

	auto bc = scene->GetBattleController();
	if (bc) {
		if (bc->GetCameraController()) {
			bc->GetCameraController()->ResetCameraRadius();
			if (bc->GetPlayer()) {
				bc->GetCameraController()->SetTargetInstant(bc->GetPlayer()->GetWorldPosition());
			}
		}
		bc->SetHudVisible(true);
		bc->TriggerGoSignal();
	}

	// プレイフェーズへ遷移
	scene->ChangePhase(std::make_unique<Play>());
}

}
