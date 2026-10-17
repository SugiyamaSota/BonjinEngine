#define NOMINMAX
#include "TutorialManager.h"
#include "battle/BattleController.h"
#include "gameObject/player/Player.h"
#include "gameObject/enemy/BaseEnemy.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "function/function.h"
#include "../../../externals/nlohmann/json.hpp"
#include <fstream>
#include <algorithm>
#include <filesystem>

#ifdef USE_IMGUI
#include "ImGuiManager.h"
#endif

namespace Bonjin {

TutorialManager::TutorialManager() = default;
TutorialManager::~TutorialManager() = default;

void TutorialManager::Initialize(const std::string& jsonFilePath) {
	jsonFilePath_ = jsonFilePath;
	currentStepIndex_ = 0;
	nextStepIndex_ = 0;
	transitionState_ = TransitionState::kNone;
	transitionTimer_ = 0.0f;
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

	// 2. テキストスプライト
	titleTextSprite_ = std::make_unique<TextSprite>();
	titleTextSprite_->Initialize();
	titleTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	detailTextSprite_ = std::make_unique<TextSprite>();
	detailTextSprite_->Initialize();
	detailTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	skipGuideTextSprite_ = std::make_unique<TextSprite>();
	skipGuideTextSprite_->Initialize();
	skipGuideTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	// JSON読み込み (失敗時はデフォルト初期化)
	if (!LoadJson(jsonFilePath_)) {
		SetupDefaultSteps();
	}

	RefreshSprites();
}

void TutorialManager::RefreshSprites() {
	if (bannerBgSprite_) {
		bannerBgSprite_->SetSize(bannerData_.size);
		bannerBgSprite_->SetTranslate(bannerData_.position);
		bannerBgSprite_->SetColor(bannerData_.color);
	}
	if (skipGuideTextSprite_) {
		skipGuideTextSprite_->SetText(bannerData_.skipGuideText, bannerData_.skipGuideFontSize, bannerData_.skipGuideColor);
		skipGuideTextSprite_->SetTranslate(bannerData_.skipGuidePosition);
	}
	UpdateBannerTexts();
}

bool TutorialManager::LoadJson(const std::string& jsonFilePath) {
	std::ifstream file(jsonFilePath);
	if (!file.is_open()) {
		return false;
	}

	nlohmann::json jsonData;
	try {
		file >> jsonData;
	} catch (...) {
		return false;
	}

	jsonFilePath_ = jsonFilePath;

	// バナー設定の読み込み
	if (jsonData.contains("banner") && jsonData["banner"].is_object()) {
		const auto& b = jsonData["banner"];
		if (b.contains("position") && b["position"].is_array() && b["position"].size() >= 2) {
			bannerData_.position = { b["position"][0].get<float>(), b["position"][1].get<float>() };
		}
		if (b.contains("size") && b["size"].is_array() && b["size"].size() >= 2) {
			bannerData_.size = { b["size"][0].get<float>(), b["size"][1].get<float>() };
		}
		if (b.contains("color") && b["color"].is_array() && b["color"].size() >= 4) {
			bannerData_.color = {
				b["color"][0].get<float>(),
				b["color"][1].get<float>(),
				b["color"][2].get<float>(),
				b["color"][3].get<float>()
			};
		}
		bannerData_.transitionDuration = b.value("transition_duration", 0.16f);
		if (b.contains("skip_guide_text")) {
			bannerData_.skipGuideText = ConvertString(b.value("skip_guide_text", ""));
		}
		if (b.contains("skip_guide_position") && b["skip_guide_position"].is_array() && b["skip_guide_position"].size() >= 2) {
			bannerData_.skipGuidePosition = { b["skip_guide_position"][0].get<float>(), b["skip_guide_position"][1].get<float>() };
		}
		bannerData_.skipGuideFontSize = b.value("skip_guide_font_size", 32);
		if (b.contains("skip_guide_color") && b["skip_guide_color"].is_array() && b["skip_guide_color"].size() >= 3) {
			bannerData_.skipGuideColor = RGB(
				b["skip_guide_color"][0].get<int>(),
				b["skip_guide_color"][1].get<int>(),
				b["skip_guide_color"][2].get<int>()
			);
		}
	}

	// ステップ設定の読み込み
	steps_.clear();
	if (jsonData.contains("steps") && jsonData["steps"].is_array()) {
		for (const auto& item : jsonData["steps"]) {
			TutorialStepData step{};
			step.id = item.value("id", "");
			step.titleText = ConvertString(item.value("title", ""));
			step.detailKeyboard = ConvertString(item.value("detail_keyboard", ""));
			step.detailGamepad = ConvertString(item.value("detail_gamepad", ""));
			step.titleFontSize = item.value("title_font_size", 32);
			step.detailFontSize = item.value("detail_font_size", 16);

			if (item.contains("title_color") && item["title_color"].is_array() && item["title_color"].size() >= 3) {
				step.titleColor = RGB(
					item["title_color"][0].get<int>(),
					item["title_color"][1].get<int>(),
					item["title_color"][2].get<int>()
				);
			}
			if (item.contains("detail_color") && item["detail_color"].is_array() && item["detail_color"].size() >= 3) {
				step.detailColor = RGB(
					item["detail_color"][0].get<int>(),
					item["detail_color"][1].get<int>(),
					item["detail_color"][2].get<int>()
				);
			}

			if (item.contains("condition") && item["condition"].is_object()) {
				step.conditionType = item["condition"].value("type", "none");
				step.conditionDuration = item["condition"].value("duration", 0.0f);
			}

			steps_.push_back(step);
		}
	}

	if (steps_.empty()) {
		SetupDefaultSteps();
	}

	if (currentStepIndex_ >= steps_.size()) {
		currentStepIndex_ = steps_.empty() ? 0 : steps_.size() - 1;
	}

	RefreshSprites();
	return true;
}

bool TutorialManager::SaveJson(const std::string& jsonFilePath) {
	std::string targetPath = jsonFilePath.empty() ? jsonFilePath_ : jsonFilePath;
	if (targetPath.empty()) {
		targetPath = "resources/data/tutorial.json";
	}

	nlohmann::json jsonData;

	// バナー設定
	jsonData["banner"] = {
		{ "position", { bannerData_.position.x, bannerData_.position.y } },
		{ "size", { bannerData_.size.x, bannerData_.size.y } },
		{ "color", { bannerData_.color.x, bannerData_.color.y, bannerData_.color.z, bannerData_.color.w } },
		{ "transition_duration", bannerData_.transitionDuration },
		{ "skip_guide_text", ConvertString(bannerData_.skipGuideText) },
		{ "skip_guide_position", { bannerData_.skipGuidePosition.x, bannerData_.skipGuidePosition.y } },
		{ "skip_guide_font_size", bannerData_.skipGuideFontSize },
		{ "skip_guide_color", {
			static_cast<int>(GetRValue(bannerData_.skipGuideColor)),
			static_cast<int>(GetGValue(bannerData_.skipGuideColor)),
			static_cast<int>(GetBValue(bannerData_.skipGuideColor))
		}}
	};

	// ステップ設定
	nlohmann::json stepsArray = nlohmann::json::array();
	for (const auto& step : steps_) {
		nlohmann::json stepItem;
		stepItem["id"] = step.id;
		stepItem["title"] = ConvertString(step.titleText);
		stepItem["detail_keyboard"] = ConvertString(step.detailKeyboard);
		stepItem["detail_gamepad"] = ConvertString(step.detailGamepad);
		stepItem["title_font_size"] = step.titleFontSize;
		stepItem["detail_font_size"] = step.detailFontSize;
		stepItem["title_color"] = {
			static_cast<int>(GetRValue(step.titleColor)),
			static_cast<int>(GetGValue(step.titleColor)),
			static_cast<int>(GetBValue(step.titleColor))
		};
		stepItem["detail_color"] = {
			static_cast<int>(GetRValue(step.detailColor)),
			static_cast<int>(GetGValue(step.detailColor)),
			static_cast<int>(GetBValue(step.detailColor))
		};
		stepItem["condition"] = {
			{ "type", step.conditionType },
			{ "duration", step.conditionDuration }
		};
		stepsArray.push_back(stepItem);
	}
	jsonData["steps"] = stepsArray;

	std::filesystem::path p(targetPath);
	if (p.has_parent_path()) {
		std::error_code ec;
		std::filesystem::create_directories(p.parent_path(), ec);
	}

	std::ofstream outFile(targetPath);
	if (!outFile.is_open()) {
		return false;
	}

	outFile << jsonData.dump(2) << std::endl;
	outFile.close();

	jsonFilePath_ = targetPath;
	return true;
}

void TutorialManager::SetupDefaultSteps() {
	bannerData_ = TutorialBannerData();
	steps_ = {
		{ "welcome", L"【TUTORIAL】基本操作をマスターしよう", L"", L"", 32, 16, RGB(255, 220, 80), RGB(220, 230, 245), "timer", 2.0f },
		{ "move", L"【STEP 1】左右に移動してみよう", L"[A] [D] キー で移動", L"左スティック で移動", 48, 32, RGB(100, 220, 255), RGB(240, 245, 255), "move", 1.2f },
		{ "jump", L"【STEP 2】ジャンプしてみよう", L"[SPACE] キー でジャンプ！", L"[A] ボタン でジャンプ！", 48, 32, RGB(100, 220, 255), RGB(240, 245, 255), "jump", 0.8f },
		{ "anchor", L"【STEP 3】アンカーを壁や足場に放とう", L"[J] キー で発射！[K] キー で 即座に移動できます", L"[X] ボタン で発射！[B] ボタン で 即座に移動できます", 48, 32, RGB(100, 220, 255), RGB(240, 245, 255), "anchor", 2.0f },
		{ "lock_on", L"【STEP 4】敵にアンカーを当ててロックオン！", L"敵にアンカーを刺すとロックオンされます", L"敵にアンカーを刺すとロックオンされます", 48, 32, RGB(255, 130, 80), RGB(255, 235, 220), "lock_on", 0.5f },
		{ "teleport_kill", L"【STEP 5】雷撃テレポートで一括撃破！", L"[L] キー でロックオンした敵へ急襲！", L"[Y] ボタン でロックオンした敵へ急襲！", 48, 32, RGB(255, 230, 50), RGB(255, 255, 200), "teleport_kill", 1.2f },
		{ "go_to_goal", L"【MISSION CLEAR!】ゴールへ向かおう", L"マップ奥にあるゴールに到達して次へ進もう！", L"マップ奥にあるゴールに到達して次へ進もう！", 48, 32, RGB(80, 255, 140), RGB(220, 255, 230), "goal", 0.0f },
		{ "completed", L"【COMPLETE】本編へ遷移します...", L"", L"", 48, 32, RGB(255, 255, 255), RGB(255, 255, 255), "none", 0.0f }
	};
}

void TutorialManager::ChangeStep(size_t nextStepIndex) {
	if (nextStepIndex >= steps_.size()) return;
	if (currentStepIndex_ == nextStepIndex && transitionState_ == TransitionState::kNone) return;

	// チュートリアル完了（最後のステップ）への遷移
	if (nextStepIndex == steps_.size() - 1) {
		currentStepIndex_ = nextStepIndex;
		nextStepIndex_ = nextStepIndex;
		transitionState_ = TransitionState::kNone;
		currentScaleY_ = 1.0f;
		UpdateBannerTexts();
		return;
	}

	// 高さを潰すアニメーションを開始
	nextStepIndex_ = nextStepIndex;
	transitionState_ = TransitionState::kClosing;
	transitionTimer_ = 0.0f;
}

void TutorialManager::ChangeStep(TutorialStep nextStep) {
	size_t targetIndex = static_cast<size_t>(nextStep);
	if (targetIndex < steps_.size()) {
		ChangeStep(targetIndex);
	}
}

bool TutorialManager::IsCompleted() const {
	if (steps_.empty()) return true;
	return currentStepIndex_ >= steps_.size() - 1;
}

TutorialStep TutorialManager::GetCurrentStep() const {
	return static_cast<TutorialStep>(currentStepIndex_);
}

void TutorialManager::UpdateBannerTexts() {
	if (currentStepIndex_ >= steps_.size()) return;

	const auto& step = steps_[currentStepIndex_];
	if (titleTextSprite_) {
		titleTextSprite_->SetText(step.titleText, step.titleFontSize, step.titleColor);
		titleTextSprite_->SetTranslate({ bannerData_.position.x + 10.0f, bannerData_.position.y + 12.0f });
	}

	// ゲームパッド接続状態に応じて詳細テキストを切り替え
	bool hasGamepad = Gamepad::GetInstance()->IsConnected();
	std::wstring detailText = (hasGamepad && !step.detailGamepad.empty()) ? step.detailGamepad : step.detailKeyboard;
	if (detailTextSprite_) {
		detailTextSprite_->SetText(detailText, step.detailFontSize, step.detailColor);
		detailTextSprite_->SetTranslate({ bannerData_.position.x + 20.0f, bannerData_.position.y + 48.0f });
	}
}

bool TutorialManager::CheckCondition(BattleController* battleController, float deltaTime) {
	if (currentStepIndex_ >= steps_.size()) return false;
	const auto& step = steps_[currentStepIndex_];
	Player* player = battleController ? battleController->GetPlayer() : nullptr;

	if (step.conditionType == "timer") {
		return stepTimer_ >= step.conditionDuration;
	}
	else if (step.conditionType == "move") {
		long lStickX = Gamepad::GetInstance()->GetLStickX();
		bool isMoving = (lStickX > 750 || lStickX < -750) ||
		                Input::GetInstance()->IsPress(DIK_A) ||
		                Input::GetInstance()->IsPress(DIK_D);
		if (isMoving) {
			moveAccumulator_ += deltaTime;
			if (moveAccumulator_ >= step.conditionDuration) {
				return true;
			}
		}
	}
	else if (step.conditionType == "jump") {
		bool jumpTrigger = Input::GetInstance()->IsTrigger(DIK_SPACE) ||
		                   Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A);
		if (jumpTrigger) {
			hasJumped_ = true;
		}
		if (hasJumped_ && stepTimer_ >= step.conditionDuration) {
			return true;
		}
	}
	else if (step.conditionType == "anchor") {
		if (player && player->HasAnchor()) {
			hasShotAnchor_ = true;
		}
		bool teleportTrigger = Input::GetInstance()->IsTrigger(DIK_K) ||
		                       Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_B);
		if (hasShotAnchor_ && (teleportTrigger || stepTimer_ >= step.conditionDuration)) {
			return true;
		}
	}
	else if (step.conditionType == "lock_on") {
		if (player && player->GetLockedOnEnemiesCount() > 0) {
			hasLockedOn_ = true;
		}
		if (hasLockedOn_ && stepTimer_ >= step.conditionDuration) {
			return true;
		}
	}
	else if (step.conditionType == "teleport_kill") {
		bool anyDefeated = false;
		if (battleController) {
			for (const auto& enemy : battleController->GetEnemies()) {
				if (enemy->GetIsDead()) {
					anyDefeated = true;
					break;
				}
			}
		}
		if (anyDefeated || (player && player->GetExp() > 0)) {
			hasKilledEnemy_ = true;
		}
		if (hasKilledEnemy_ && stepTimer_ >= step.conditionDuration) {
			return true;
		}
	}
	else if (step.conditionType == "goal") {
		if (battleController && battleController->IsGoalReached()) {
			return true;
		}
	}

	return false;
}

void TutorialManager::Update(BattleController* battleController, float deltaTime) {
	stepTimer_ += deltaTime;

	// スキップキー判定 (TABキー)
	if (Input::GetInstance()->IsTrigger(DIK_TAB)) {
		ChangeStep(steps_.size() - 1);
		return;
	}

	// -------------------------------------------------------------
	// 1. トランジション（高さ潰し・復元）アニメーションの更新
	// -------------------------------------------------------------
	if (transitionState_ == TransitionState::kClosing) {
		transitionTimer_ += deltaTime;
		float duration = (std::max)(0.01f, bannerData_.transitionDuration);
		float progress = (std::clamp)(transitionTimer_ / duration, 0.0f, 1.0f);

		if (progress >= 1.0f) {
			// 高さが完全に潰れた瞬間！
			currentScaleY_ = 0.0f;
			currentStepIndex_ = nextStepIndex_; // ステップを更新
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
		float duration = (std::max)(0.01f, bannerData_.transitionDuration);
		float progress = (std::clamp)(transitionTimer_ / duration, 0.0f, 1.0f);

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
	if (transitionState_ == TransitionState::kNone && currentStepIndex_ < steps_.size() - 1) {
		if (CheckCondition(battleController, deltaTime)) {
			ChangeStep(currentStepIndex_ + 1);
		}
	}

	// -------------------------------------------------------------
	// 3. バナースプライトのサイズ・座標更新（中心基準で潰す）
	// -------------------------------------------------------------
	float currentHeight = bannerData_.size.y * currentScaleY_;
	float offsetY = (bannerData_.size.y - currentHeight) * 0.5f;

	if (bannerBgSprite_) {
		bannerBgSprite_->SetSize({ bannerData_.size.x, currentHeight });
		bannerBgSprite_->SetTranslate({ bannerData_.position.x, bannerData_.position.y + offsetY });
	}

	// テキストの透明度と位置（潰れている間は文字をフェードアウト）
	float textAlpha = 0.0f;
	if (currentScaleY_ > 0.35f) {
		textAlpha = (currentScaleY_ - 0.35f) / 0.65f;
	}

	if (titleTextSprite_) {
		titleTextSprite_->SetColor({ 1.0f, 1.0f, 1.0f, textAlpha });
		titleTextSprite_->SetTranslate({ bannerData_.position.x + 10.0f, bannerData_.position.y + offsetY + 12.0f * currentScaleY_ });
	}

	if (detailTextSprite_) {
		detailTextSprite_->SetColor({ 1.0f, 1.0f, 1.0f, textAlpha });
		detailTextSprite_->SetTranslate({ bannerData_.position.x + 30.0f, bannerData_.position.y + offsetY + 48.0f * currentScaleY_ });
	}

	if (bannerBgSprite_) bannerBgSprite_->Update();
	if (titleTextSprite_) titleTextSprite_->Update();
	if (detailTextSprite_) detailTextSprite_->Update();
	if (skipGuideTextSprite_) skipGuideTextSprite_->Update();
}

void TutorialManager::Draw() {
	if (bannerBgSprite_) bannerBgSprite_->Draw();
	if (titleTextSprite_) titleTextSprite_->Draw();
	if (detailTextSprite_) detailTextSprite_->Draw();
	if (skipGuideTextSprite_) skipGuideTextSprite_->Draw();
}

void TutorialManager::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Tutorial Manager (Status)")) {
		ImGui::Text("Current Step: %zu / %zu", currentStepIndex_, steps_.size());
		ImGui::Text("Step Timer: %.2fs", stepTimer_);
		ImGui::Text("ScaleY: %.2f", currentScaleY_);
		ImGui::TreePop();
	}
#endif
}

} // namespace Bonjin
