#define NOMINMAX
#include "TutorialManager.h"
#include "../game/BattleController.h"
#include "../game/gameObject/player/Player.h"
#include "../game/gameObject/enemy/BaseEnemy.h"
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

namespace {

void ColorrefToFloat3(COLORREF color, float outCol[3]) {
	outCol[0] = static_cast<float>(GetRValue(color)) / 255.0f;
	outCol[1] = static_cast<float>(GetGValue(color)) / 255.0f;
	outCol[2] = static_cast<float>(GetBValue(color)) / 255.0f;
}

COLORREF Float3ToColorref(const float col[3]) {
	int r = (std::clamp)(static_cast<int>(col[0] * 255.0f + 0.5f), 0, 255);
	int g = (std::clamp)(static_cast<int>(col[1] * 255.0f + 0.5f), 0, 255);
	int b = (std::clamp)(static_cast<int>(col[2] * 255.0f + 0.5f), 0, 255);
	return RGB(r, g, b);
}

#ifdef USE_IMGUI
bool InputTextWString(const char* label, std::wstring& wstr, ImGuiInputTextFlags flags = 0) {
	std::string u8Str = ConvertString(wstr);
	char buf[512] = {};
	strncpy_s(buf, u8Str.c_str(), sizeof(buf) - 1);
	if (ImGui::InputText(label, buf, sizeof(buf), flags)) {
		wstr = ConvertString(std::string(buf));
		return true;
	}
	return false;
}

bool InputTextStdString(const char* label, std::string& str, ImGuiInputTextFlags flags = 0) {
	char buf[256] = {};
	strncpy_s(buf, str.c_str(), sizeof(buf) - 1);
	if (ImGui::InputText(label, buf, sizeof(buf), flags)) {
		str = std::string(buf);
		return true;
	}
	return false;
}
#endif

} // namespace

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
	fileCheckTimer_ = 0.0f;
	statusMessageTimer_ = 0.0f;

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
		RefreshSprites();
	}
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

	// ファイル更新日時を記録
	std::error_code ec;
	if (std::filesystem::exists(jsonFilePath_, ec)) {
		lastFileWriteTime_ = std::filesystem::last_write_time(jsonFilePath_, ec);
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

	// 親ディレクトリが存在しない場合は作成
	std::filesystem::path p(targetPath);
	if (p.has_parent_path()) {
		std::error_code ec;
		std::filesystem::create_directories(p.parent_path(), ec);
	}

	std::ofstream outFile(targetPath);
	if (!outFile.is_open()) {
		statusMessage_ = "Failed to open file: " + targetPath;
		statusMessageTimer_ = 3.0f;
		return false;
	}

	outFile << jsonData.dump(2) << std::endl;
	outFile.close();

	jsonFilePath_ = targetPath;

	// 書き込み日時を更新して二重リロード防止
	std::error_code ec;
	if (std::filesystem::exists(targetPath, ec)) {
		lastFileWriteTime_ = std::filesystem::last_write_time(targetPath, ec);
	}

	statusMessage_ = "Saved to " + targetPath;
	statusMessageTimer_ = 3.0f;
	return true;
}

bool TutorialManager::ReloadJson() {
	bool ok = LoadJson(jsonFilePath_);
	if (ok) {
		statusMessage_ = "Reloaded " + jsonFilePath_;
	} else {
		statusMessage_ = "Reload Failed: " + jsonFilePath_;
	}
	statusMessageTimer_ = 3.0f;
	return ok;
}

void TutorialManager::CheckHotReload() {
	if (!autoReloadEnabled_ || jsonFilePath_.empty()) {
		return;
	}

	std::error_code ec;
	if (!std::filesystem::exists(jsonFilePath_, ec)) {
		return;
	}

	auto currentWriteTime = std::filesystem::last_write_time(jsonFilePath_, ec);
	if (!ec && currentWriteTime != lastFileWriteTime_) {
		LoadJson(jsonFilePath_);
		statusMessage_ = "Hot-Reloaded: " + jsonFilePath_;
		statusMessageTimer_ = 3.0f;
	}
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
	if (statusMessageTimer_ > 0.0f) {
		statusMessageTimer_ -= deltaTime;
	}

	fileCheckTimer_ += deltaTime;
	if (fileCheckTimer_ >= 0.5f) {
		fileCheckTimer_ = 0.0f;
		CheckHotReload();
	}

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
	if (ImGui::TreeNode("Tutorial Manager & JSON Editor")) {
		// -------------------------------------------------------------
		// 1. ファイル操作 (Save / Reload / Hot Reload)
		// -------------------------------------------------------------
		if (ImGui::CollapsingHeader("File Operations", ImGuiTreeNodeFlags_DefaultOpen)) {
			InputTextStdString("JSON Path", jsonFilePath_);

			if (ImGui::Button("Save JSON")) {
				SaveJson(jsonFilePath_);
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload JSON")) {
				ReloadJson();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset to Default")) {
				SetupDefaultSteps();
				RefreshSprites();
				statusMessage_ = "Reset to default steps.";
				statusMessageTimer_ = 3.0f;
			}

			ImGui::Checkbox("Auto-Reload on File Change", &autoReloadEnabled_);

			if (statusMessageTimer_ > 0.0f && !statusMessage_.empty()) {
				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "%s", statusMessage_.c_str());
			}
		}

		// -------------------------------------------------------------
		// 2. 進行・プレビュー制御
		// -------------------------------------------------------------
		if (ImGui::CollapsingHeader("Playback & Step Control", ImGuiTreeNodeFlags_DefaultOpen)) {
			int currentIdx = static_cast<int>(currentStepIndex_);
			std::vector<std::string> stepNames;
			std::vector<const char*> stepNamePtrs;
			for (size_t i = 0; i < steps_.size(); ++i) {
				std::string titleStr = ConvertString(steps_[i].titleText);
				stepNames.push_back(std::to_string(i) + ": [" + steps_[i].id + "] " + titleStr);
				stepNamePtrs.push_back(stepNames.back().c_str());
			}

			if (!stepNamePtrs.empty()) {
				if (ImGui::Combo("Current Step", &currentIdx, stepNamePtrs.data(), static_cast<int>(stepNamePtrs.size()))) {
					ChangeStep(static_cast<size_t>(currentIdx));
				}
			}

			if (ImGui::Button("Prev Step")) {
				if (currentStepIndex_ > 0) {
					ChangeStep(currentStepIndex_ - 1);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Next Step")) {
				if (currentStepIndex_ < steps_.size() - 1) {
					ChangeStep(currentStepIndex_ + 1);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Restart")) {
				ChangeStep(static_cast<size_t>(0));
			}
			ImGui::SameLine();
			if (ImGui::Button("Skip to Complete")) {
				ChangeStep(steps_.size() - 1);
			}

			ImGui::Text("Step Timer: %.2fs", stepTimer_);
			ImGui::Text("ScaleY: %.2f", currentScaleY_);
		}

		// -------------------------------------------------------------
		// 3. バナー外観・共通設定
		// -------------------------------------------------------------
		if (ImGui::CollapsingHeader("Banner Appearance")) {
			bool changed = false;

			if (ImGui::DragFloat2("Position", &bannerData_.position.x, 1.0f, 0.0f, 1920.0f)) {
				changed = true;
			}
			if (ImGui::DragFloat2("Size", &bannerData_.size.x, 1.0f, 10.0f, 1920.0f)) {
				changed = true;
			}
			if (ImGui::ColorEdit4("BG Color", &bannerData_.color.x)) {
				changed = true;
			}
			ImGui::SliderFloat("Transition Duration", &bannerData_.transitionDuration, 0.01f, 1.0f, "%.2fs");

			ImGui::Separator();
			ImGui::Text("Skip Guide Settings");
			if (InputTextWString("Guide Text", bannerData_.skipGuideText)) {
				changed = true;
			}
			if (ImGui::DragFloat2("Guide Position", &bannerData_.skipGuidePosition.x, 1.0f, 0.0f, 1920.0f)) {
				changed = true;
			}
			if (ImGui::SliderInt("Guide Font Size", &bannerData_.skipGuideFontSize, 8, 72)) {
				changed = true;
			}

			float guideCol[3];
			ColorrefToFloat3(bannerData_.skipGuideColor, guideCol);
			if (ImGui::ColorEdit3("Guide Color", guideCol)) {
				bannerData_.skipGuideColor = Float3ToColorref(guideCol);
				changed = true;
			}

			if (changed) {
				RefreshSprites();
			}
		}

		// -------------------------------------------------------------
		// 4. ステップ一覧・個別編集
		// -------------------------------------------------------------
		if (ImGui::CollapsingHeader("Tutorial Steps List", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("+ Add New Step")) {
				TutorialStepData newStep{};
				newStep.id = "new_step_" + std::to_string(steps_.size());
				newStep.titleText = L"新規ステップタイトル";
				newStep.detailKeyboard = L"キーボード操作説明";
				newStep.detailGamepad = L"ゲームパッド操作説明";
				newStep.titleFontSize = 48;
				newStep.detailFontSize = 32;
				newStep.titleColor = RGB(100, 220, 255);
				newStep.detailColor = RGB(240, 245, 255);
				newStep.conditionType = "none";
				newStep.conditionDuration = 1.0f;
				steps_.push_back(newStep);
				UpdateBannerTexts();
			}

			ImGui::Separator();

			size_t stepToDelete = static_cast<size_t>(-1);
			size_t stepToMoveUp = static_cast<size_t>(-1);
			size_t stepToMoveDown = static_cast<size_t>(-1);
			size_t stepToDuplicate = static_cast<size_t>(-1);

			for (size_t i = 0; i < steps_.size(); ++i) {
				auto& step = steps_[i];
				std::string titleStr = ConvertString(step.titleText);
				std::string headerLabel = "[" + std::to_string(i) + "] " + step.id + " : " + titleStr + "###step_" + std::to_string(i);

				ImGui::PushID(static_cast<int>(i));
				if (ImGui::TreeNode(headerLabel.c_str())) {
					bool stepChanged = false;

					// 操作ボタン
					if (i > 0) {
						if (ImGui::Button("Move Up")) stepToMoveUp = i;
						ImGui::SameLine();
					}
					if (i + 1 < steps_.size()) {
						if (ImGui::Button("Move Down")) stepToMoveDown = i;
						ImGui::SameLine();
					}
					if (ImGui::Button("Duplicate")) {
						stepToDuplicate = i;
					}
					ImGui::SameLine();
					if (steps_.size() > 1) {
						if (ImGui::Button("Delete Step")) {
							stepToDelete = i;
						}
						ImGui::SameLine();
					}
					if (ImGui::Button("Jump To This Step")) {
						ChangeStep(i);
					}

					ImGui::Separator();

					// ID
					InputTextStdString("ID", step.id);

					// タイトル
					if (InputTextWString("Title Text", step.titleText)) {
						stepChanged = true;
					}

					// 詳細テキスト
					if (InputTextWString("Detail (KB)", step.detailKeyboard)) {
						stepChanged = true;
					}
					if (InputTextWString("Detail (Gamepad)", step.detailGamepad)) {
						stepChanged = true;
					}

					// フォントサイズ
					if (ImGui::SliderInt("Title Font Size", &step.titleFontSize, 8, 96)) {
						stepChanged = true;
					}
					if (ImGui::SliderInt("Detail Font Size", &step.detailFontSize, 8, 72)) {
						stepChanged = true;
					}

					// カラー
					float tCol[3], dCol[3];
					ColorrefToFloat3(step.titleColor, tCol);
					ColorrefToFloat3(step.detailColor, dCol);

					if (ImGui::ColorEdit3("Title Color", tCol)) {
						step.titleColor = Float3ToColorref(tCol);
						stepChanged = true;
					}
					if (ImGui::ColorEdit3("Detail Color", dCol)) {
						step.detailColor = Float3ToColorref(dCol);
						stepChanged = true;
					}

					// 条件タイプ
					const char* conditionTypes[] = { "none", "timer", "move", "jump", "anchor", "lock_on", "teleport_kill", "goal" };
					int condIdx = 0;
					for (int c = 0; c < IM_ARRAYSIZE(conditionTypes); ++c) {
						if (step.conditionType == conditionTypes[c]) {
							condIdx = c;
							break;
						}
					}
					if (ImGui::Combo("Condition Type", &condIdx, conditionTypes, IM_ARRAYSIZE(conditionTypes))) {
						step.conditionType = conditionTypes[condIdx];
					}

					ImGui::DragFloat("Condition Duration (s)", &step.conditionDuration, 0.1f, 0.0f, 30.0f, "%.1fs");

					if (stepChanged && currentStepIndex_ == i) {
						UpdateBannerTexts();
					}

					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			// ステップ移動・削除・複製の適用
			if (stepToDelete != static_cast<size_t>(-1) && steps_.size() > 1) {
				steps_.erase(steps_.begin() + stepToDelete);
				if (currentStepIndex_ >= steps_.size()) {
					currentStepIndex_ = steps_.size() - 1;
				}
				UpdateBannerTexts();
			}
			else if (stepToMoveUp != static_cast<size_t>(-1) && stepToMoveUp > 0) {
				std::swap(steps_[stepToMoveUp], steps_[stepToMoveUp - 1]);
				if (currentStepIndex_ == stepToMoveUp) currentStepIndex_--;
				else if (currentStepIndex_ == stepToMoveUp - 1) currentStepIndex_++;
				UpdateBannerTexts();
			}
			else if (stepToMoveDown != static_cast<size_t>(-1) && stepToMoveDown + 1 < steps_.size()) {
				std::swap(steps_[stepToMoveDown], steps_[stepToMoveDown + 1]);
				if (currentStepIndex_ == stepToMoveDown) currentStepIndex_++;
				else if (currentStepIndex_ == stepToMoveDown + 1) currentStepIndex_--;
				UpdateBannerTexts();
			}
			else if (stepToDuplicate != static_cast<size_t>(-1)) {
				TutorialStepData dup = steps_[stepToDuplicate];
				dup.id += "_copy";
				steps_.insert(steps_.begin() + stepToDuplicate + 1, dup);
				UpdateBannerTexts();
			}
		}

		ImGui::TreePop();
	}
#endif
}

} // namespace Bonjin
