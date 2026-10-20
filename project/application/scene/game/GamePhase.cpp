#include "GamePhase.h"
#include "GameScene.h"

#include "../result/ResultScene.h"
#include "SceneManager.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "vector.h"

#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace Bonjin {

// ===================================================================
// GamePhaseHighlight
// ===================================================================

void GamePhaseHighlight::SetupHighlightDissolve() {
	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->AddPostEffect(PostEffect::kDigitalDissolve);
	sm->SetDissolveMode(dissolveMode_);
	sm->SetDissolveThreshold(0.0f); // 真っ黒から実体化スタート
	sm->SetDissolveEdgeWidth(0.06f); // サイバー発光エッジ
	sm->SetDissolveEdgeColor({ 0.0f, 1.5f, 2.0f }); // サイバーブルー
}

void GamePhaseHighlight::Enter(GameScene* scene) {
	subState_ = SubState::kFadeIn;
	currentHighlightIndex_ = 0;
	timer_ = 0.0f;

	auto bc = scene->GetBattleController();
	if (!bc) {
		return;
	}

	// 演出中はHUDを非表示
	bc->SetHudVisible(false);

	const auto& highlights = bc->GetMapChipField()->GetHighlights();
	if (!highlights.empty()) {
		// 最初のハイライト位置へカメラを瞬時にワープ
		bc->GetCameraController()->SetTargetInstant(highlights[0].startWorldPos);
	}

	// 最初のカメラ位置に合わせて描画用モデルを初期更新
	bc->UpdateVisuals(0.0f);

	// 初回ハイライトのDigitalDissolveを開始
	SetupHighlightDissolve();
}

void GamePhaseHighlight::Update(GameScene* scene, float deltaTime) {
	auto sm = SceneManager::GetInstance();
	auto bc = scene->GetBattleController();
	if (!bc) {
		return;
	}

	// スキップ判定 (SPACEキー または GAMEPAD Aボタン)
	// ※ シーン切り替え直後の入力残りや連打による誤スキップを防ぐため0.2秒のガードを設ける
	if (timer_ > 0.2f && (Input::GetInstance()->IsTrigger(DIK_SPACE) || 
		Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A))) {
		FinishHighlight(scene);
		return;
	}

	const auto& highlights = bc->GetMapChipField()->GetHighlights();
	timer_ += deltaTime;

	switch (subState_) {
	case SubState::kFadeIn: {
		float t = std::clamp(timer_ / kFadeDuration, 0.0f, 1.0f);
		sm->SetFadeAmount(t); // 徐々に実体化（0.0 -> 1.0）

		if (timer_ >= kFadeDuration) {
			sm->ClearPostEffects(); // パン中は通常描画（エフェクトなしで鮮明に）
			sm->SetFadeAmount(1.0f);
			subState_ = SubState::kCameraPan;
			timer_ = 0.0f;
		}
		break;
	}
	case SubState::kCameraPan: {
		if (currentHighlightIndex_ < highlights.size()) {
			const auto& currentHighlight = highlights[currentHighlightIndex_];
			float duration = (std::max)(currentHighlight.displayDuration, 0.1f);
			float t = std::clamp(timer_ / duration, 0.0f, 1.0f);

			// スムーズな補間 (3t^2 - 2t^3)
			float smoothT = t * t * (3.0f - 2.0f * t);
			Vector3 targetPos = Add(currentHighlight.startWorldPos, Multiply(smoothT, currentHighlight.panDirection));
			bc->GetCameraController()->Update(targetPos, deltaTime);

			if (timer_ >= duration) {
				// 3つ目（または最後のハイライト）に達した場合はフェードアウトせずにカメラを引きながらプレイヤーへ移動
				bool isLastHighlight = (currentHighlightIndex_ >= 2 || currentHighlightIndex_ + 1 >= highlights.size());
				if (isLastHighlight) {
					subState_ = SubState::kReturnToPlayer;
					returnStartPos_ = targetPos;
					timer_ = 0.0f;
					// 画面は明るい状態（FadeAmount = 1.0f）を維持
					sm->ClearPostEffects();
					sm->SetFadeAmount(1.0f);
					sm->SetDissolveThreshold(1.0f);
				} else {
					// 次のハイライトへ向けて暗転フェードアウト開始
					sm->ClearPostEffects();
					sm->AddPostEffect(PostEffect::kSimpleFade);
					sm->SetFadeAmount(1.0f);
					subState_ = SubState::kFadeOut;
					timer_ = 0.0f;
				}
			}
		} else {
			subState_ = SubState::kReturnToPlayer;
			timer_ = 0.0f;
		}
		break;
	}
	case SubState::kReturnToPlayer: {
		float t = std::clamp(timer_ / kReturnDuration, 0.0f, 1.0f);

		// なめらかな補間カーブ (SmootherStep: 6t^5 - 15t^4 + 10t^3)
		float smoothT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

		Vector3 playerPos = bc->GetPlayer() ? bc->GetPlayer()->GetWorldPosition() : Vector3{ 0.0f, 0.0f, 0.0f };
		Vector3 currentTarget = Lerp(returnStartPos_, playerPos, smoothT);

		// カメラ注視点をプレイヤー位置へ向けて更新
		bc->GetCameraController()->SetTargetInstant(currentTarget);

		// 移動中にカメラを引き（ズームアウト）にし、プレイヤーに近づくにつれて通常距離に戻す
		float pullFactor = std::sin(t * 3.1415926535f);
		float currentRadius = Camera::kDefaultRadius + (kMaxPullBackRadius - Camera::kDefaultRadius) * pullFactor;
		bc->GetCameraController()->SetCameraRadius(currentRadius);

		if (timer_ >= kReturnDuration) {
			// プレイヤーに到着完了！カメラ距離を確実に通常に戻し、GOシグナルを発行
			bc->GetCameraController()->ResetCameraRadius();
			bc->GetCameraController()->SetTargetInstant(playerPos);
			FinishHighlight(scene);
			return;
		}
		break;
	}
	case SubState::kFadeOut: {
		float t = 1.0f - std::clamp(timer_ / kFadeDuration, 0.0f, 1.0f);
		sm->SetFadeAmount(t); // 徐々に暗くなる

		if (timer_ >= kFadeDuration) {
			currentHighlightIndex_++;
			timer_ = 0.0f;

			if (currentHighlightIndex_ < highlights.size()) {
				// 次のハイライトへカメラを瞬時に移動させて Fade In
				bc->GetCameraController()->SetTargetInstant(highlights[currentHighlightIndex_].startWorldPos);
				bc->UpdateVisuals(0.0f);

				subState_ = SubState::kFadeIn;
			} else {
				subState_ = SubState::kReturnToPlayer;
				if (bc->GetPlayer()) {
					returnStartPos_ = bc->GetPlayer()->GetWorldPosition();
				}
			}
		}
		break;
	}
	case SubState::kPlayerFocusIn: {
		// プレイヤー出現は DigitalDissolve（電子ブロック・実体化演出）
		float t = std::clamp(timer_ / kPlayerFocusDuration, 0.0f, 1.0f);
		sm->SetDissolveThreshold(t);

		if (bc->GetPlayer()) {
			bc->GetCameraController()->Update(bc->GetPlayer()->GetWorldPosition(), deltaTime);
		}

		if (timer_ >= kPlayerFocusDuration) {
			FinishHighlight(scene);
			return;
		}
		break;
	}
	default:
		break;
	}

	// 描画用モデル（カメラ追従WVP行列、天球、ブロック、ゴール、パーティクル等）を更新
	bc->UpdateVisuals(deltaTime);
}

void GamePhaseHighlight::DrawImGui(GameScene* scene) {
#ifdef USE_IMGUI
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "=== Stage Intro Highlight ===");
	auto bc = scene->GetBattleController();
	if (bc && bc->GetMapChipField()) {
		if (subState_ == SubState::kReturnToPlayer) {
			ImGui::Text("State: Return to Player (Pull-back Camera)");
		} else {
			ImGui::Text("Highlight: %zu / %zu", currentHighlightIndex_ + 1, bc->GetMapChipField()->GetHighlights().size());
		}
	}
	if (ImGui::Button("Skip Highlight")) {
		FinishHighlight(scene);
	}
	ImGui::Separator();
#else
	(void)scene;
#endif
}

void GamePhaseHighlight::Exit(GameScene* scene) {
	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->SetFadeAmount(1.0f);
	sm->SetDissolveThreshold(1.0f);

	auto bc = scene->GetBattleController();
	if (bc && bc->GetCameraController()) {
		bc->GetCameraController()->ResetCameraRadius();
	}
}

void GamePhaseHighlight::FinishHighlight(GameScene* scene) {
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
	scene->ChangePhase(std::make_unique<GamePhasePlay>());
}

// ===================================================================
// GamePhasePlay
// ===================================================================

void GamePhasePlay::Enter(GameScene* scene) {
	auto bc = scene->GetBattleController();
	if (bc) {
		bc->SetHudVisible(true);
		if (!bc->IsGoSignalActive()) {
			bc->TriggerGoSignal();
		}
	}
}

void GamePhasePlay::Update(GameScene* scene, float deltaTime) {
	auto bc = scene->GetBattleController();
	if (!bc) {
		return;
	}

	bc->Update(deltaTime);

	if (bc->IsGoalReached()) {
		ResultScene::SetIsClear(true);
		scene->ChangePhase(std::make_unique<GamePhaseGoal>());
		SceneManager::GetInstance()->RequestSceneChange("ResultScene");
	} else if (bc->GetPlayer() && bc->GetPlayer()->GetStatus().IsDead()) {
		ResultScene::SetIsClear(false);
		scene->ChangePhase(std::make_unique<GamePhaseGoal>());
		SceneManager::GetInstance()->RequestSceneChange("ResultScene");
	}
}

void GamePhasePlay::Exit(GameScene* scene) {
	(void)scene;
}

// ===================================================================
// GamePhaseGoal
// ===================================================================

void GamePhaseGoal::Enter(GameScene* scene) {
	(void)scene;
}

void GamePhaseGoal::Update(GameScene* scene, float deltaTime) {
	(void)scene;
	(void)deltaTime;
	// リザルトシーンへの遷移待ち
}

void GamePhaseGoal::Exit(GameScene* scene) {
	(void)scene;
}

}
