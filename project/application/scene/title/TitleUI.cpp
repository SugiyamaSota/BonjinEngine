#include "TitleUI.h"
#include <cmath>
#include <random>
#include <algorithm>

using namespace Bonjin;

void TitleUI::Initialize() {
	timer_ = 0.0f;
	glitchTimer_ = 0.0f;
	isGlitching_ = false;

	// メインタイトル文字
	titleText_ = std::make_unique<TextSprite>();
	titleText_->Initialize();
	titleText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	titleText_->SetText(L"A N C H O R", 88, RGB(235, 250, 255));
	titleText_->SetTranslate({ 640.0f, 220.0f });

	// スタート案内テキスト「[ PRESS SPACE OR 'A' BUTTON TO DIVE ]」
	startText_ = std::make_unique<TextSprite>();
	startText_->Initialize();
	startText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	startText_->SetText(L"[ PRESS SPACE OR 'A' BUTTON TO DIVE ]", 28, RGB(210, 240, 255));
	startText_->SetTranslate({ 640.0f, 520.0f });
}

void TitleUI::Unload() {
	titleText_.reset();
	startText_.reset();
}

void TitleUI::Update(float deltaTime, bool isDiving, float diveProgress) {
	timer_ += deltaTime;

	// グリッチ演出（待機中のみ）
	if (!isDiving) {
		UpdateGlitch(deltaTime);
	}

	if (titleText_) {
		titleText_->Update();
	}
	if (startText_) {
		float alpha = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(timer_ * 4.0f));
		if (isDiving) {
			alpha *= (std::max)(0.0f, 1.0f - diveProgress * 1.8f);
		}
		startText_->SetColor({ 0.9f, 0.95f, 1.0f, alpha });
		startText_->Update();
	}

	// ダイブ遷移中のフェードアウト
	if (isDiving) {
		float uiAlpha = (std::max)(0.0f, 1.0f - diveProgress * 1.8f);
		if (titleText_) titleText_->SetColor({ 1.0f, 1.0f, 1.0f, uiAlpha });
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
					titleText_->SetTranslate({ 640.0f + offsetX, 220.0f + offsetY });
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
				titleText_->SetTranslate({ 640.0f, 220.0f });
				titleText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			}
		}
	}
}

void TitleUI::SetAlpha(float alpha) {
	if (titleText_) titleText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	if (startText_) startText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
}

void TitleUI::Draw() {
	if (titleText_) {
		titleText_->Draw();
	}
	if (startText_) {
		startText_->Draw();
	}
}
