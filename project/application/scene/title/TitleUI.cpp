#include "TitleUI.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include <cmath>
#include <random>
#include <algorithm>

using namespace Bonjin;

void TitleUI::Initialize() {
	timer_ = 0.0f;
	glitchTimer_ = 0.0f;
	isGlitching_ = false;
	selectedItem_ = TitleMenuItem::kGameStart;

	// メインタイトル文字
	titleText_ = std::make_unique<TextSprite>();
	titleText_->Initialize();
	titleText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	titleText_->SetText(L"A N C H O R", 84, RGB(235, 250, 255));
	titleText_->SetTranslate({ 640.0f, 210.0f });

	// メニュー項目1: GAME START
	gameStartText_ = std::make_unique<TextSprite>();
	gameStartText_->Initialize();
	gameStartText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	gameStartText_->SetText(L"> GAME START <", 36, RGB(0, 245, 255));
	gameStartText_->SetTranslate({ 640.0f, 460.0f });

	// メニュー項目2: TUTORIAL
	tutorialText_ = std::make_unique<TextSprite>();
	tutorialText_->Initialize();
	tutorialText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	tutorialText_->SetText(L"  TUTORIAL  ", 36, RGB(110, 150, 180));
	tutorialText_->SetTranslate({ 640.0f, 530.0f });

	// 操作案内テキスト
	guideText_ = std::make_unique<TextSprite>();
	guideText_->Initialize();
	guideText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	guideText_->SetText(L"[ W/S/UP/DOWN: SELECT    SPACE/'A': DIVE ]", 22, RGB(180, 220, 245));
	guideText_->SetTranslate({ 640.0f, 620.0f });
}

void TitleUI::Unload() {
	titleText_.reset();
	gameStartText_.reset();
	tutorialText_.reset();
	guideText_.reset();
}

void TitleUI::Update(float deltaTime, bool isDiving, float diveProgress) {
	timer_ += deltaTime;

	// グリッチ演出（待機中のみ）
	if (!isDiving) {
		UpdateGlitch(deltaTime);
		UpdateMenuSelection();
	}

	if (titleText_) {
		titleText_->Update();
	}

	float pulse = 0.5f + 0.5f * std::sin(timer_ * 5.0f);
	float fadeOut = isDiving ? (std::max)(0.0f, 1.0f - diveProgress * 2.0f) : 1.0f;

	// メニュー項目の外観更新
	if (gameStartText_) {
		bool isSelected = (selectedItem_ == TitleMenuItem::kGameStart);
		float alpha = (isSelected ? (0.85f + 0.15f * pulse) : 0.45f) * fadeOut;
		float scale = isSelected ? 1.08f : 0.95f;
		gameStartText_->SetScale({ scale, scale });
		gameStartText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		gameStartText_->Update();
	}

	if (tutorialText_) {
		bool isSelected = (selectedItem_ == TitleMenuItem::kTutorial);
		float alpha = (isSelected ? (0.85f + 0.15f * pulse) : 0.45f) * fadeOut;
		float scale = isSelected ? 1.08f : 0.95f;
		tutorialText_->SetScale({ scale, scale });
		tutorialText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		tutorialText_->Update();
	}

	// ガイドテキスト
	if (guideText_) {
		float guideAlpha = (0.4f + 0.4f * (0.5f + 0.5f * std::sin(timer_ * 3.5f))) * fadeOut;
		guideText_->SetColor({ 0.8f, 0.9f, 1.0f, guideAlpha });
		guideText_->Update();
	}

	// タイトル文字のダイブフェードアウト
	if (isDiving && titleText_) {
		titleText_->SetColor({ 1.0f, 1.0f, 1.0f, fadeOut });
	}
}

void TitleUI::UpdateMenuSelection() {
	auto input = Input::GetInstance();
	auto gamepad = Gamepad::GetInstance();

	bool moveUp = input->IsTrigger(DIK_UP) || input->IsTrigger(DIK_W) || gamepad->IsTrigger(XINPUT_GAMEPAD_DPAD_UP);
	bool moveDown = input->IsTrigger(DIK_DOWN) || input->IsTrigger(DIK_S) || gamepad->IsTrigger(XINPUT_GAMEPAD_DPAD_DOWN);

	if (moveUp || moveDown) {
		if (selectedItem_ == TitleMenuItem::kGameStart) {
			selectedItem_ = TitleMenuItem::kTutorial;
		} else {
			selectedItem_ = TitleMenuItem::kGameStart;
		}

		// テキストの切り替え（矢印マーカー）
		if (selectedItem_ == TitleMenuItem::kGameStart) {
			gameStartText_->SetText(L"> GAME START <", 36, RGB(0, 245, 255));
			tutorialText_->SetText(L"  TUTORIAL  ", 36, RGB(110, 150, 180));
		} else {
			gameStartText_->SetText(L"  GAME START  ", 36, RGB(110, 150, 180));
			tutorialText_->SetText(L"> TUTORIAL <", 36, RGB(0, 245, 255));
		}
	}
}

void TitleUI::UpdateGlitch(float deltaTime) {
	glitchTimer_ -= deltaTime;

	if (glitchTimer_ <= 0.0f) {
		if (!isGlitching_) {
			// 一定確率でグリッチ発生（0.06秒間の瞬間ノイズ）
			if ((rand() % 100) < 35) {
				isGlitching_ = true;
				glitchTimer_ = 0.06f;
				if (titleText_) {
					float offsetX = static_cast<float>((rand() % 16) - 8);
					float offsetY = static_cast<float>((rand() % 8) - 4);
					titleText_->SetTranslate({ 640.0f + offsetX, 210.0f + offsetY });
					titleText_->SetColor({ 0.4f, 1.0f, 1.0f, 1.0f });
				}
			} else {
				glitchTimer_ = 0.4f + static_cast<float>(rand() % 100) / 100.0f * 1.5f;
			}
		} else {
			// グリッチ終了・通常位置と色に復帰
			isGlitching_ = false;
			glitchTimer_ = 1.0f + static_cast<float>(rand() % 100) / 100.0f * 2.0f;
			if (titleText_) {
				titleText_->SetTranslate({ 640.0f, 210.0f });
				titleText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			}
		}
	}
}

void TitleUI::SetAlpha(float alpha) {
	if (titleText_) titleText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	if (gameStartText_) gameStartText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	if (tutorialText_) tutorialText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	if (guideText_) guideText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
}

void TitleUI::Draw() {
	if (titleText_) {
		titleText_->Draw();
	}
	if (gameStartText_) {
		gameStartText_->Draw();
	}
	if (tutorialText_) {
		tutorialText_->Draw();
	}
	if (guideText_) {
		guideText_->Draw();
	}
}
