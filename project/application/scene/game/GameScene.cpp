#include "GameScene.h"

#include "gameObject/player/PlayerStatusRepository.h"
#include "../result/ResultScene.h"
#include "SceneManager.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "vector.h"
#include <algorithm>

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{
	camera_ = camera;

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = "GameScene";
	nextSceneType_ = "GameScene";

	// プレイヤーステータスの読み込み
	PlayerStatusRepository::GetInstance()->Load("resources/data/player_status.json");

	battleController_ = std::make_unique<BattleController>();
	battleController_->Initialize(camera_, "resources/maps/tutorial.json");

	// ハイライトデータが存在する場合は kHighlight フェーズを開始
	const auto& highlights = battleController_->GetMapChipField()->GetHighlights();
	if (!highlights.empty()) {
		phase_ = GamePhase::kHighlight;
		highlightState_ = HighlightState::kDissolveIn;
		currentHighlightIndex_ = 0;
		highlightTimer_ = 0.0f;

		// 演出中はHUDを非表示
		battleController_->SetHudVisible(false);

		// 最初のハイライト位置へカメラを瞬時にワープ
		battleController_->GetCameraController()->SetTargetInstant(highlights[0].startWorldPos);

		// 最初のカメラ位置に合わせて描画用モデルを初期更新
		battleController_->UpdateVisuals(0.0f);

		// DigitalDissolve ポストエフェクトを有効化（真っ黒スタート）
		auto sm = SceneManager::GetInstance();
		sm->AddPostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
		sm->SetDissolveThreshold(0.0f);
		sm->SetDissolveEdgeWidth(0.06f);
		sm->SetDissolveEdgeColor({ 0.0f, 1.5f, 2.0f });
	} else {
		phase_ = GamePhase::kPlay;
		battleController_->SetHudVisible(true);
	}
}

void GameScene::Unload() {
	if (phase_ == GamePhase::kHighlight) {
		FinishHighlight();
	}
	if (battleController_) {
		battleController_->Unload();
		battleController_.reset();
	}
}

void GameScene::Update(float deltaTime) {
	switch (phase_) {
	case GamePhase::kHighlight:
		UpdateHighlightPhase(deltaTime);
		break;

	case GamePhase::kPlay:
		UpdatePlayPhase(deltaTime);
		break;

	case GamePhase::kGoal:
		// リザルトシーンへの遷移待ち
		break;
	}
}

void GameScene::UpdateHighlightPhase(float deltaTime) {
	auto sm = SceneManager::GetInstance();

	// スキップ判定 (SPACEキー または GAMEPAD Aボタン)
	if (Input::GetInstance()->IsTrigger(DIK_SPACE) || 
		Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A)) {
		FinishHighlight();
		return;
	}

	const auto& highlights = battleController_->GetMapChipField()->GetHighlights();
	highlightTimer_ += deltaTime;

	switch (highlightState_) {
	case HighlightState::kDissolveIn: {
		highlightState_ = HighlightState::kCameraPan;
		highlightTimer_ = 0.0f;
		break;
	}
	case HighlightState::kCameraPan: {
		if (currentHighlightIndex_ < highlights.size()) {
			const auto& currentHighlight = highlights[currentHighlightIndex_];
			float duration = (std::max)(currentHighlight.displayDuration, 0.1f);
			float t = std::clamp(highlightTimer_ / duration, 0.0f, 1.0f);

			// スムーズな補間 (3t^2 - 2t^3)
			float smoothT = t * t * (3.0f - 2.0f * t);
			Vector3 targetPos = Add(currentHighlight.startWorldPos, Multiply(smoothT, currentHighlight.panDirection));
			battleController_->GetCameraController()->Update(targetPos, deltaTime);

			if (highlightTimer_ >= duration) {
				highlightState_ = HighlightState::kDissolveOut;
				highlightTimer_ = 0.0f;
			}
		} else {
			highlightState_ = HighlightState::kPlayerFocusIn;
			highlightTimer_ = 0.0f;
		}
		break;
	}
	case HighlightState::kDissolveOut: {
		float t = 1.0f - std::clamp(highlightTimer_ / kDissolveDuration, 0.0f, 1.0f);
		sm->SetDissolveThreshold(t);

		if (highlightTimer_ >= kDissolveDuration) {
			currentHighlightIndex_++;
			highlightTimer_ = 0.0f;

			if (currentHighlightIndex_ < highlights.size()) {
				// 次のハイライトへカメラを瞬時に移動させて Dissolve In
				battleController_->GetCameraController()->SetTargetInstant(highlights[currentHighlightIndex_].startWorldPos);
				highlightState_ = HighlightState::kDissolveIn;
			} else {
				// 全ハイライト完了 -> プレイヤーへカメラを瞬時に移動させて最終 Dissolve In
				if (battleController_->GetPlayer()) {
					battleController_->GetCameraController()->SetTargetInstant(battleController_->GetPlayer()->GetWorldPosition());
				}
				highlightState_ = HighlightState::kPlayerFocusIn;
			}
		}
		break;
	}
	case HighlightState::kPlayerFocusIn: {
		float t = std::clamp(highlightTimer_ / kDissolveDuration, 0.0f, 1.0f);
		sm->SetDissolveThreshold(t);

		if (battleController_->GetPlayer()) {
			battleController_->GetCameraController()->Update(battleController_->GetPlayer()->GetWorldPosition(), deltaTime);
		}

		if (highlightTimer_ >= kDissolveDuration) {
			FinishHighlight();
		}
		break;
	}
	default:
		break;
	}

	// 描画用モデル（カメラ追従WVP行列、天球、ブロック、ゴール、パーティクル等）を更新
	battleController_->UpdateVisuals(deltaTime);
}

void GameScene::FinishHighlight() {
	auto sm = SceneManager::GetInstance();
	sm->SetDissolveThreshold(1.0f);
	sm->RemovePostEffect(DirectXCommon::PostEffect::kDigitalDissolve);

	if (battleController_ && battleController_->GetPlayer()) {
		battleController_->GetCameraController()->SetTargetInstant(battleController_->GetPlayer()->GetWorldPosition());
	}
	if (battleController_) {
		battleController_->SetHudVisible(true);
	}
	ChangePhase(GamePhase::kPlay);
}

void GameScene::UpdatePlayPhase(float deltaTime) {
	battleController_->Update(deltaTime);

	if (battleController_->IsGoalReached()) {
		ResultScene::SetIsClear(true);
		ChangePhase(GamePhase::kGoal);
		SceneManager::GetInstance()->RequestSceneChange("ResultScene");
	} else if (battleController_->GetPlayer() && battleController_->GetPlayer()->GetStatus().IsDead()) {
		ResultScene::SetIsClear(false);
		ChangePhase(GamePhase::kGoal);
		SceneManager::GetInstance()->RequestSceneChange("ResultScene");
	}
}

void GameScene::Draw() {
	battleController_->Draw();
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}

void GameScene::DrawSceneImGui() {
#ifdef USE_IMGUI
	if (phase_ == GamePhase::kHighlight) {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "=== Stage Intro Highlight ===");
		if (battleController_ && battleController_->GetMapChipField()) {
			ImGui::Text("Highlight: %zu / %zu", currentHighlightIndex_ + 1, battleController_->GetMapChipField()->GetHighlights().size());
		}
		if (ImGui::Button("Skip Highlight")) {
			FinishHighlight();
		}
		ImGui::Separator();
	}

	if (battleController_) {
		battleController_->DrawImGui();
	}
#endif
}