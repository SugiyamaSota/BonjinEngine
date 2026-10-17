#define NOMINMAX
#include "TutorialEditor.h"
#include "TutorialManager.h"
#include "function/function.h"
#include <algorithm>
#include <vector>

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

TutorialEditor::TutorialEditor() = default;
TutorialEditor::~TutorialEditor() = default;

void TutorialEditor::Initialize(TutorialManager* tutorialManager) {
	tutorialManager_ = tutorialManager;
	if (tutorialManager_) {
		jsonFilePath_ = tutorialManager_->GetJsonFilePath();
	}

	std::error_code ec;
	if (std::filesystem::exists(jsonFilePath_, ec)) {
		lastFileWriteTime_ = std::filesystem::last_write_time(jsonFilePath_, ec);
	}
}

void TutorialEditor::Update(float deltaTime) {
	if (statusMessageTimer_ > 0.0f) {
		statusMessageTimer_ -= deltaTime;
	}

	fileCheckTimer_ += deltaTime;
	if (fileCheckTimer_ >= 0.5f) {
		fileCheckTimer_ = 0.0f;
		CheckHotReload();
	}
}

void TutorialEditor::SetStatus(const std::string& msg, float duration) {
	statusMessage_ = msg;
	statusMessageTimer_ = duration;
}

bool TutorialEditor::Save() {
	if (!tutorialManager_) return false;

	bool ok = tutorialManager_->SaveJson(jsonFilePath_);
	if (ok) {
		std::error_code ec;
		if (std::filesystem::exists(jsonFilePath_, ec)) {
			lastFileWriteTime_ = std::filesystem::last_write_time(jsonFilePath_, ec);
		}
		SetStatus("Saved to " + jsonFilePath_);
	} else {
		SetStatus("Failed to save: " + jsonFilePath_);
	}
	return ok;
}

bool TutorialEditor::Reload() {
	if (!tutorialManager_) return false;

	bool ok = tutorialManager_->LoadJson(jsonFilePath_);
	if (ok) {
		std::error_code ec;
		if (std::filesystem::exists(jsonFilePath_, ec)) {
			lastFileWriteTime_ = std::filesystem::last_write_time(jsonFilePath_, ec);
		}
		SetStatus("Reloaded: " + jsonFilePath_);
	} else {
		SetStatus("Failed to reload: " + jsonFilePath_);
	}
	return ok;
}

void TutorialEditor::CheckHotReload() {
	if (!autoReloadEnabled_ || !tutorialManager_ || jsonFilePath_.empty()) {
		return;
	}

	std::error_code ec;
	if (!std::filesystem::exists(jsonFilePath_, ec)) {
		return;
	}

	auto currentWriteTime = std::filesystem::last_write_time(jsonFilePath_, ec);
	if (!ec && currentWriteTime != lastFileWriteTime_) {
		Reload();
		SetStatus("Hot-Reloaded: " + jsonFilePath_);
	}
}

void TutorialEditor::DrawImGui() {
#ifdef USE_IMGUI
	if (!tutorialManager_) return;

	if (ImGui::TreeNode("Tutorial Editor")) {
		// -------------------------------------------------------------
		// 1. ファイル操作 (Save / Reload / Hot Reload)
		// -------------------------------------------------------------
		if (ImGui::CollapsingHeader("File Operations", ImGuiTreeNodeFlags_DefaultOpen)) {
			InputTextStdString("JSON Path", jsonFilePath_);

			if (ImGui::Button("Save JSON")) {
				Save();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload JSON")) {
				Reload();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset to Default")) {
				tutorialManager_->SetupDefaultSteps();
				tutorialManager_->RefreshSprites();
				SetStatus("Reset to default steps.");
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
			auto& steps = tutorialManager_->GetSteps();
			size_t currentStepIdx = tutorialManager_->GetCurrentStepIndex();
			int currentIdx = static_cast<int>(currentStepIdx);

			std::vector<std::string> stepNames;
			std::vector<const char*> stepNamePtrs;
			for (size_t i = 0; i < steps.size(); ++i) {
				std::string titleStr = ConvertString(steps[i].titleText);
				stepNames.push_back(std::to_string(i) + ": [" + steps[i].id + "] " + titleStr);
				stepNamePtrs.push_back(stepNames.back().c_str());
			}

			if (!stepNamePtrs.empty()) {
				if (ImGui::Combo("Current Step", &currentIdx, stepNamePtrs.data(), static_cast<int>(stepNamePtrs.size()))) {
					tutorialManager_->ChangeStep(static_cast<size_t>(currentIdx));
				}
			}

			if (ImGui::Button("Prev Step")) {
				if (currentStepIdx > 0) {
					tutorialManager_->ChangeStep(currentStepIdx - 1);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Next Step")) {
				if (currentStepIdx < steps.size() - 1) {
					tutorialManager_->ChangeStep(currentStepIdx + 1);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Restart")) {
				tutorialManager_->ChangeStep(static_cast<size_t>(0));
			}
			ImGui::SameLine();
			if (ImGui::Button("Skip to Complete")) {
				tutorialManager_->ChangeStep(steps.size() - 1);
			}
		}

		// -------------------------------------------------------------
		// 3. バナー外観設定
		// -------------------------------------------------------------
		if (ImGui::CollapsingHeader("Banner Appearance")) {
			auto& bannerData = tutorialManager_->GetBannerData();
			bool changed = false;

			if (ImGui::DragFloat2("Position", &bannerData.position.x, 1.0f, 0.0f, 1920.0f)) {
				changed = true;
			}
			if (ImGui::DragFloat2("Size", &bannerData.size.x, 1.0f, 10.0f, 1920.0f)) {
				changed = true;
			}
			if (ImGui::ColorEdit4("BG Color", &bannerData.color.x)) {
				changed = true;
			}
			ImGui::SliderFloat("Transition Duration", &bannerData.transitionDuration, 0.01f, 1.0f, "%.2fs");

			ImGui::Separator();
			ImGui::Text("Skip Guide Settings");
			if (InputTextWString("Guide Text", bannerData.skipGuideText)) {
				changed = true;
			}
			if (ImGui::DragFloat2("Guide Position", &bannerData.skipGuidePosition.x, 1.0f, 0.0f, 1920.0f)) {
				changed = true;
			}
			if (ImGui::SliderInt("Guide Font Size", &bannerData.skipGuideFontSize, 8, 72)) {
				changed = true;
			}

			float guideCol[3];
			ColorrefToFloat3(bannerData.skipGuideColor, guideCol);
			if (ImGui::ColorEdit3("Guide Color", guideCol)) {
				bannerData.skipGuideColor = Float3ToColorref(guideCol);
				changed = true;
			}

			if (changed) {
				tutorialManager_->RefreshSprites();
			}
		}

		// -------------------------------------------------------------
		// 4. ステップ一覧・個別編集
		// -------------------------------------------------------------
		if (ImGui::CollapsingHeader("Tutorial Steps List", ImGuiTreeNodeFlags_DefaultOpen)) {
			auto& steps = tutorialManager_->GetSteps();
			size_t currentStepIdx = tutorialManager_->GetCurrentStepIndex();

			if (ImGui::Button("+ Add New Step")) {
				TutorialStepData newStep{};
				newStep.id = "new_step_" + std::to_string(steps.size());
				newStep.titleText = L"新規ステップタイトル";
				newStep.detailKeyboard = L"キーボード操作説明";
				newStep.detailGamepad = L"ゲームパッド操作説明";
				newStep.titleFontSize = 48;
				newStep.detailFontSize = 32;
				newStep.titleColor = RGB(100, 220, 255);
				newStep.detailColor = RGB(240, 245, 255);
				newStep.conditionType = "none";
				newStep.conditionDuration = 1.0f;
				steps.push_back(newStep);
				tutorialManager_->RefreshSprites();
			}

			ImGui::Separator();

			size_t stepToDelete = static_cast<size_t>(-1);
			size_t stepToMoveUp = static_cast<size_t>(-1);
			size_t stepToMoveDown = static_cast<size_t>(-1);
			size_t stepToDuplicate = static_cast<size_t>(-1);

			for (size_t i = 0; i < steps.size(); ++i) {
				auto& step = steps[i];
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
					if (i + 1 < steps.size()) {
						if (ImGui::Button("Move Down")) stepToMoveDown = i;
						ImGui::SameLine();
					}
					if (ImGui::Button("Duplicate")) {
						stepToDuplicate = i;
					}
					ImGui::SameLine();
					if (steps.size() > 1) {
						if (ImGui::Button("Delete Step")) {
							stepToDelete = i;
						}
						ImGui::SameLine();
					}
					if (ImGui::Button("Jump To This Step")) {
						tutorialManager_->ChangeStep(i);
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

					if (stepChanged && currentStepIdx == i) {
						tutorialManager_->RefreshSprites();
					}

					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			// ステップ移動・削除・複製の適用
			if (stepToDelete != static_cast<size_t>(-1) && steps.size() > 1) {
				steps.erase(steps.begin() + stepToDelete);
				if (currentStepIdx >= steps.size()) {
					tutorialManager_->ChangeStep(steps.size() - 1);
				} else {
					tutorialManager_->RefreshSprites();
				}
			}
			else if (stepToMoveUp != static_cast<size_t>(-1) && stepToMoveUp > 0) {
				std::swap(steps[stepToMoveUp], steps[stepToMoveUp - 1]);
				if (currentStepIdx == stepToMoveUp) tutorialManager_->ChangeStep(currentStepIdx - 1);
				else if (currentStepIdx == stepToMoveUp - 1) tutorialManager_->ChangeStep(currentStepIdx + 1);
				else tutorialManager_->RefreshSprites();
			}
			else if (stepToMoveDown != static_cast<size_t>(-1) && stepToMoveDown + 1 < steps.size()) {
				std::swap(steps[stepToMoveDown], steps[stepToMoveDown + 1]);
				if (currentStepIdx == stepToMoveDown) tutorialManager_->ChangeStep(currentStepIdx + 1);
				else if (currentStepIdx == stepToMoveDown + 1) tutorialManager_->ChangeStep(currentStepIdx - 1);
				else tutorialManager_->RefreshSprites();
			}
			else if (stepToDuplicate != static_cast<size_t>(-1)) {
				TutorialStepData dup = steps[stepToDuplicate];
				dup.id += "_copy";
				steps.insert(steps.begin() + stepToDuplicate + 1, dup);
				tutorialManager_->RefreshSprites();
			}
		}

		ImGui::TreePop();
	}
#endif
}

} // namespace Bonjin
