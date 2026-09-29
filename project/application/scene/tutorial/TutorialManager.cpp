#define NOMINMAX
#include "TutorialManager.h"
#include "../game/BattleController.h"
#include "../game/gameObject/player/Player.h"
#include "../game/gameObject/enemy/BaseEnemy.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "ImGuiManager.h"
#endif

namespace Bonjin {

TutorialManager::TutorialManager() = default;
TutorialManager::~TutorialManager() = default;

void TutorialManager::Initialize() {
	currentStep_ = TutorialStep::kWelcome;
	nextStep_ = TutorialStep::kWelcome;
	transitionState_ = TransitionState::kNone;
	transitionTimer_ = 0.0f;
	transitionDuration_ = 0.16f;
	currentScaleY_ = 1.0f;

	stepTimer_ = 0.0f;
	moveAccumulator_ = 0.0f;
	hasJumped_ = false;
	hasShotAnchor_ = false;
	hasLockedOn_ = false;
	hasKilledEnemy_ = false;

	// 1. ミッションバナー背景 (中央上)
	bannerBgSprite_ = std::make_unique<Sprite>();
	bannerBgSprite_->Initialize("default.png");
	bannerBgSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });
	bannerBgSprite_->SetSize(bannerSize_);
	bannerBgSprite_->SetTranslate(bannerPosition_);
	bannerBgSprite_->SetColor({ 0.04f, 0.07f, 0.12f, 0.85f });

	// 3. テキストスプライト
	titleTextSprite_ = std::make_unique<TextSprite>();
	titleTextSprite_->Initialize();
	titleTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	detailTextSprite_ = std::make_unique<TextSprite>();
	detailTextSprite_->Initialize();
	detailTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	skipGuideTextSprite_ = std::make_unique<TextSprite>();
	skipGuideTextSprite_->Initialize();
	skipGuideTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });
	skipGuideTextSprite_->SetText(L"[TAB]: チュートリアルをスキップ", 32, RGB(160, 170, 190));
	skipGuideTextSprite_->SetTranslate({ 1020.0f, 25.0f });

	UpdateBannerTexts();
}

void TutorialManager::ChangeStep(TutorialStep nextStep) {
	if (currentStep_ == nextStep && transitionState_ == TransitionState::kNone) {
		return;
	}

	// チュートリアル完了への即時スキップ
	if (nextStep == TutorialStep::kCompleted) {
		currentStep_ = TutorialStep::kCompleted;
		nextStep_ = TutorialStep::kCompleted;
		transitionState_ = TransitionState::kNone;
		currentScaleY_ = 1.0f;
		UpdateBannerTexts();
		return;
	}

	// 高さを潰すアニメーションを開始
	nextStep_ = nextStep;
	transitionState_ = TransitionState::kClosing;
	transitionTimer_ = 0.0f;
}

void TutorialManager::UpdateBannerTexts() {
	switch (currentStep_) {
	case TutorialStep::kWelcome:
		titleTextSprite_->SetText(L"【TUTORIAL】基本操作をマスターしよう", 32, RGB(255, 220, 80));
		detailTextSprite_->SetText(L"", 16, RGB(220, 230, 245));
		break;

	case TutorialStep::kMove:
		titleTextSprite_->SetText(L"【STEP 1】左右に移動してみよう", 48, RGB(100, 220, 255));
		detailTextSprite_->SetText(L"[A] [D] キー で移動", 32, RGB(240, 245, 255));
		break;

	case TutorialStep::kJump:
		titleTextSprite_->SetText(L"【STEP 2】ジャンプしてみよう", 48, RGB(100, 220, 255));
		detailTextSprite_->SetText(L"[SPACE] キー でジャンプ！", 32, RGB(240, 245, 255));
		break;

	case TutorialStep::kAnchor:
		titleTextSprite_->SetText(L"【STEP 3】アンカーを壁や足場に放とう", 48, RGB(100, 220, 255));
		detailTextSprite_->SetText(L"[J] キー で発射！[K] キー で 即座に移動できます", 32, RGB(240, 245, 255));
		break;

	case TutorialStep::kLockOn:
		titleTextSprite_->SetText(L"【STEP 4】敵にアンカーを当ててロックオン！", 48, RGB(255, 130, 80));
		detailTextSprite_->SetText(L"敵にアンカーを刺すとロックオンされます", 32, RGB(255, 235, 220));
		break;

	case TutorialStep::kTeleportKill:
		titleTextSprite_->SetText(L"【STEP 5】雷撃テレポートで一括撃破！", 48, RGB(255, 230, 50));
		detailTextSprite_->SetText(L"[L] キー でロックオンした敵へ急襲！", 32, RGB(255, 255, 200));
		break;

	case TutorialStep::kGoToGoal:
		titleTextSprite_->SetText(L"【MISSION CLEAR!】ゴールへ向かおう", 48, RGB(80, 255, 140));
		detailTextSprite_->SetText(L"マップ奥にあるゴールに到達して次へ進もう！", 32, RGB(220, 255, 230));
		break;

	case TutorialStep::kCompleted:
		titleTextSprite_->SetText(L"【COMPLETE】本編へ遷移します...", 48, RGB(255, 255, 255));
		detailTextSprite_->SetText(L"", 32, RGB(255, 255, 255));
		break;
	}

	titleTextSprite_->SetTranslate({ bannerPosition_.x + 10.0f, bannerPosition_.y + 12.0f });
	detailTextSprite_->SetTranslate({ bannerPosition_.x + 20.0f, bannerPosition_.y + 48.0f });
}

void TutorialManager::Update(BattleController* battleController, float deltaTime) {
	if (!battleController) return;

	Player* player = battleController->GetPlayer();
	stepTimer_ += deltaTime;

	// スキップキー判定 (TABキー)
	if (Input::GetInstance()->IsTrigger(DIK_TAB)) {
		ChangeStep(TutorialStep::kCompleted);
		return;
	}

	// -------------------------------------------------------------
	// 1. トランジション（高さ潰し・復元）アニメーションの更新
	// -------------------------------------------------------------
	if (transitionState_ == TransitionState::kClosing) {
		transitionTimer_ += deltaTime;
		float progress = (std::clamp)(transitionTimer_ / transitionDuration_, 0.0f, 1.0f);

		if (progress >= 1.0f) {
			// 高さが完全に潰れた瞬間！
			currentScaleY_ = 0.0f;
			currentStep_ = nextStep_; // ステップを更新
			stepTimer_ = 0.0f;
			moveAccumulator_ = 0.0f;
			hasJumped_ = false;
			hasShotAnchor_ = false;
			hasLockedOn_ = false;
			hasKilledEnemy_ = false;

			UpdateBannerTexts(); // テキストを切り替え

			// 復元アニメーションへ
			transitionState_ = TransitionState::kOpening;
			transitionTimer_ = 0.0f;
		} else {
			// EaseInQuad: 滑らかに潰す
			currentScaleY_ = 1.0f - (progress * progress);
		}
	}
	else if (transitionState_ == TransitionState::kOpening) {
		transitionTimer_ += deltaTime;
		float progress = (std::clamp)(transitionTimer_ / transitionDuration_, 0.0f, 1.0f);

		if (progress >= 1.0f) {
			currentScaleY_ = 1.0f;
			transitionState_ = TransitionState::kNone;
		} else {
			// EaseOutQuad: 滑らかに開く
			float inv = 1.0f - progress;
			currentScaleY_ = 1.0f - (inv * inv);
		}
	} else {
		currentScaleY_ = 1.0f;
	}

	// -------------------------------------------------------------
	// 2. 各ステップの進行判定（トランジション中以外に判定）
	// -------------------------------------------------------------
	if (transitionState_ == TransitionState::kNone) {
		switch (currentStep_) {
		case TutorialStep::kWelcome:
			if (stepTimer_ >= 2.0f) {
				ChangeStep(TutorialStep::kMove);
			}
			break;

		case TutorialStep::kMove: {
			long lStickX = Gamepad::GetInstance()->GetLStickX();
			bool isMoving = (lStickX > 750 || lStickX < -750) ||
							Input::GetInstance()->IsPress(DIK_A) ||
							Input::GetInstance()->IsPress(DIK_D);
			if (isMoving) {
				moveAccumulator_ += deltaTime;
				if (moveAccumulator_ >= 1.2f) {
					ChangeStep(TutorialStep::kJump);
				}
			}
			break;
		}

		case TutorialStep::kJump: {
			bool jumpTrigger = Input::GetInstance()->IsTrigger(DIK_SPACE);
			if (jumpTrigger) {
				hasJumped_ = true;
			}
			if (hasJumped_ && stepTimer_ >= 0.8f) {
				ChangeStep(TutorialStep::kAnchor);
			}
			break;
		}

		case TutorialStep::kAnchor: {
			if (player && player->HasAnchor()) {
				hasShotAnchor_ = true;
			}
			bool teleportTrigger = Input::GetInstance()->IsTrigger(DIK_K) ||
			                       Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_B);
			if (hasShotAnchor_ && (teleportTrigger || stepTimer_ >= 2.0f)) {
				ChangeStep(TutorialStep::kLockOn);
			}
			break;
		}

		case TutorialStep::kLockOn: {
			if (player && player->GetLockedOnEnemiesCount() > 0) {
				hasLockedOn_ = true;
			}
			if (hasLockedOn_ && stepTimer_ >= 0.5f) {
				ChangeStep(TutorialStep::kTeleportKill);
			}
			break;
		}

		case TutorialStep::kTeleportKill: {
			bool anyDefeated = false;
			for (const auto& enemy : battleController->GetEnemies()) {
				if (enemy->GetIsDead()) {
					anyDefeated = true;
					break;
				}
			}
			if (anyDefeated || (player && player->GetExp() > 0)) {
				hasKilledEnemy_ = true;
			}
			if (hasKilledEnemy_ && stepTimer_ >= 1.2f) {
				ChangeStep(TutorialStep::kGoToGoal);
			}
			break;
		}

		case TutorialStep::kGoToGoal: {
			if (battleController->IsGoalReached()) {
				ChangeStep(TutorialStep::kCompleted);
			}
			break;
		}

		case TutorialStep::kCompleted:
			break;
		}
	}

	// -------------------------------------------------------------
	// 3. バナースプライトのサイズ・座標更新（中心基準で潰す）
	// -------------------------------------------------------------
	float currentHeight = bannerSize_.y * currentScaleY_;
	float offsetY = (bannerSize_.y - currentHeight) * 0.5f;

	bannerBgSprite_->SetSize({ bannerSize_.x, currentHeight });
	bannerBgSprite_->SetTranslate({ bannerPosition_.x, bannerPosition_.y + offsetY });

	// テキストの透明度と位置（潰れている間は文字をフェードアウト）
	float textAlpha = 0.0f;
	if (currentScaleY_ > 0.35f) {
		textAlpha = (currentScaleY_ - 0.35f) / 0.65f;
	}

	titleTextSprite_->SetColor({ 1.0f, 1.0f, 1.0f, textAlpha });
	detailTextSprite_->SetColor({ 1.0f, 1.0f, 1.0f, textAlpha });

	titleTextSprite_->SetTranslate({ bannerPosition_.x + 10.0f, bannerPosition_.y + offsetY + 12.0f * currentScaleY_ });
	detailTextSprite_->SetTranslate({ bannerPosition_.x + 30.0f, bannerPosition_.y + offsetY + 48.0f * currentScaleY_ });

	bannerBgSprite_->Update();
	titleTextSprite_->Update();
	detailTextSprite_->Update();
	skipGuideTextSprite_->Update();
}

void TutorialManager::Draw() {
	if (bannerBgSprite_) bannerBgSprite_->Draw();
	if (titleTextSprite_) titleTextSprite_->Draw();
	if (detailTextSprite_) detailTextSprite_->Draw();
	if (skipGuideTextSprite_) skipGuideTextSprite_->Draw();
}

void TutorialManager::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Tutorial Manager Debug")) {
		const char* stepNames[] = {
			"0: Welcome",
			"1: Move",
			"2: Jump",
			"3: Anchor",
			"4: LockOn",
			"5: TeleportKill",
			"6: GoToGoal",
			"7: Completed"
		};
		int currentIdx = static_cast<int>(currentStep_);
		if (ImGui::Combo("Current Step", &currentIdx, stepNames, IM_ARRAYSIZE(stepNames))) {
			ChangeStep(static_cast<TutorialStep>(currentIdx));
		}
		if (ImGui::Button("Next Step")) {
			if (currentIdx < 7) {
				ChangeStep(static_cast<TutorialStep>(currentIdx + 1));
			}
		}
		if (ImGui::Button("Skip to Complete")) {
			ChangeStep(TutorialStep::kCompleted);
		}
		ImGui::SliderFloat("Transition Duration", &transitionDuration_, 0.05f, 0.5f, "%.2fs");
		ImGui::Text("ScaleY: %.2f", currentScaleY_);
		ImGui::DragFloat2("Banner Pos", &bannerPosition_.x, 1.0f, 0.0f, 1280.0f);
		ImGui::TreePop();
	}
#endif
}

}
