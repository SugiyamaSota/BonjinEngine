#define NOMINMAX
#include "AnchorStockIndicator.h"
#include "../gameObject/player/Player.h"
#include "Camera.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#ifdef USE_IMGUI
#include "ImGuiManager.h"
#endif

namespace Bonjin {

namespace {
	const Vector3 kReadyColor = { 0.35f, 1.0f, 0.55f };  // ストックあり（グリーン）
	const Vector3 kEmptyColor = { 1.0f, 0.28f, 0.25f };  // ストックなし（レッド）

	const float kBurstDuration = 0.35f;
	const float kPopDuration = 0.25f;
	const float kShakeDuration = 0.3f;
	const float kDeniedFlashDuration = 0.35f;
	const float kBumpDuration = 0.12f;

	float EaseOutBack(float t) {
		const float c1 = 1.70158f;
		const float c3 = c1 + 1.0f;
		float u = t - 1.0f;
		return 1.0f + c3 * u * u * u + c1 * u * u;
	}

	Vector3 Lerp3(const Vector3& a, const Vector3& b, float t) {
		return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
	}

	std::unique_ptr<Sprite> CreateCircleSprite(float size) {
		auto sprite = std::make_unique<Sprite>();
		sprite->Initialize("circle.png");
		sprite->SetAnchor({ 0.5f, 0.5f, 0.0f });
		sprite->SetSize({ size, size });
		sprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		return sprite;
	}
}

AnchorStockIndicator::AnchorStockIndicator() = default;
AnchorStockIndicator::~AnchorStockIndicator() = default;

void AnchorStockIndicator::Initialize() {
	barBgSprite_ = std::make_unique<Sprite>();
	barBgSprite_->Initialize("default.png");
	barBgSprite_->SetAnchor({ 0.0f, 0.5f, 0.0f });
	barBgSprite_->SetColor({ 0.05f, 0.08f, 0.12f, 0.0f });

	barFillSprite_ = std::make_unique<Sprite>();
	barFillSprite_->Initialize("default.png");
	barFillSprite_->SetAnchor({ 0.0f, 0.5f, 0.0f });
	barFillSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	labelSprite_ = std::make_unique<TextSprite>();
	labelSprite_->Initialize();
	labelSprite_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	SetLabel(L"READY");

	pips_.clear();
	isVisible_ = false;
	prevStock_ = -1;
}

void AnchorStockIndicator::EnsurePipCount(int count) {
	while (static_cast<int>(pips_.size()) < count) {
		Pip pip;
		pip.glow = CreateCircleSprite(glowSize_);
		pip.core = CreateCircleSprite(pipSize_);
		pip.burst = CreateCircleSprite(pipSize_);
		pips_.push_back(std::move(pip));
	}
	if (static_cast<int>(pips_.size()) > count) {
		pips_.resize(count);
	}
}

void AnchorStockIndicator::SetLabel(const std::wstring& text) {
	if (text == cachedLabel_) {
		return;
	}
	cachedLabel_ = text;
	// 白で生成し、マテリアルカラーで着色する
	labelSprite_->SetText(text, 16, RGB(255, 255, 255));
}

void AnchorStockIndicator::Update(Player* player, Camera* camera, float deltaTime) {
	if (!player || !camera) {
		return;
	}

	const int stock = player->GetAnchorStock();
	const int maxStock = player->GetMaxAnchorStock();
	EnsurePipCount(maxStock);

	// --- 入力要求の受け取り ---
	bool denied = false;
	if (player->ConsumeAnchorIndicatorRequest(denied) && enabled_) {
		if (!isVisible_) {
			isVisible_ = true;
			showTimer_ = 0.0f;
		} else {
			bumpTimer_ = kBumpDuration;
		}
		holdRemaining_ = holdDuration_;
		fadeTimer_ = 0.0f;
		if (denied) {
			shakeTimer_ = kShakeDuration;
			deniedFlash_ = kDeniedFlashDuration;
		}
	}

	// --- ストック増減の検知（玉のバースト / ポップ演出） ---
	if (prevStock_ >= 0 && isVisible_) {
		if (stock < prevStock_) {
			for (int i = stock; i < prevStock_ && i < maxStock; ++i) {
				pips_[i].burstTimer = kBurstDuration;
				pips_[i].burstIsGain = false;
			}
		} else if (stock > prevStock_) {
			for (int i = prevStock_; i < stock && i < maxStock; ++i) {
				pips_[i].burstTimer = kBurstDuration;
				pips_[i].burstIsGain = true;
				pips_[i].popTimer = kPopDuration;
			}
		}
	}
	prevStock_ = stock;

	if (!isVisible_) {
		return;
	}

	// --- 表示タイマー進行 ---
	showTimer_ += deltaTime;
	shakeTimer_ = (std::max)(0.0f, shakeTimer_ - deltaTime);
	deniedFlash_ = (std::max)(0.0f, deniedFlash_ - deltaTime);
	bumpTimer_ = (std::max)(0.0f, bumpTimer_ - deltaTime);

	float alpha = 1.0f;
	float groupScale = 1.0f;
	float riseOffset = 0.0f;

	if (showTimer_ < popInDuration_) {
		float p = std::clamp(showTimer_ / popInDuration_, 0.0f, 1.0f);
		groupScale = 0.5f + 0.5f * EaseOutBack(p);
		alpha = p;
		riseOffset = (1.0f - p) * 10.0f;
	} else if (holdRemaining_ > 0.0f) {
		holdRemaining_ -= deltaTime;
	} else {
		fadeTimer_ += deltaTime;
		float p = std::clamp(fadeTimer_ / fadeDuration_, 0.0f, 1.0f);
		alpha = 1.0f - p;
		riseOffset = -p * 12.0f;
		if (p >= 1.0f) {
			isVisible_ = false;
			return;
		}
	}
	if (bumpTimer_ > 0.0f) {
		groupScale *= 1.0f + 0.12f * (bumpTimer_ / kBumpDuration);
	}

	// --- 画面座標（プレイヤー頭上） ---
	Vector3 worldPos = player->GetWorldPosition();
	worldPos.y += worldOffsetY_;
	Vector3 screen = camera->Project(worldPos);

	float shakeX = 0.0f;
	if (shakeTimer_ > 0.0f) {
		float k = shakeTimer_ / kShakeDuration;
		shakeX = std::sin(shakeTimer_ * 70.0f) * 6.0f * k;
	}
	const float cx = screen.x + shakeX;
	const float cy = screen.y + riseOffset;

	// --- 配色 ---
	const bool hasStock = stock > 0;
	Vector3 theme = hasStock ? kReadyColor : kEmptyColor;
	// 拒否時は白寄りに一瞬フラッシュ
	if (deniedFlash_ > 0.0f) {
		theme = Lerp3(theme, { 1.0f, 0.85f, 0.85f }, deniedFlash_ / kDeniedFlashDuration * 0.6f);
	}
	const float breathe = 0.85f + 0.15f * std::sin(showTimer_ * 8.0f);

	// --- 玉（ストック） ---
	const float startX = cx - (maxStock - 1) * 0.5f * pipSpacing_ * groupScale;
	for (int i = 0; i < maxStock; ++i) {
		Pip& pip = pips_[i];
		const bool filled = i < stock;
		Vector2 pos = { startX + i * pipSpacing_ * groupScale, cy };

		float popScale = 1.0f;
		if (pip.popTimer > 0.0f) {
			pip.popTimer = (std::max)(0.0f, pip.popTimer - deltaTime);
			popScale = 1.0f + 0.6f * (pip.popTimer / kPopDuration);
		}
		const float s = groupScale * popScale;

		Vector3 coreColor;
		float coreAlpha;
		float glowAlpha;
		if (filled) {
			coreColor = Lerp3(theme, { 1.0f, 1.0f, 1.0f }, 0.35f);
			coreAlpha = 1.0f;
			glowAlpha = 0.5f * breathe;
		} else {
			coreColor = { theme.x * 0.35f, theme.y * 0.35f, theme.z * 0.35f };
			coreAlpha = 0.65f;
			glowAlpha = hasStock ? 0.0f : 0.22f * breathe; // 0ストック時は空玉も赤く滲ませる
		}

		pip.glow->SetTranslate(pos);
		pip.glow->SetSize({ glowSize_, glowSize_ });
		pip.glow->SetScale({ s, s });
		pip.glow->SetColor({ theme.x, theme.y, theme.z, glowAlpha * alpha });
		pip.glow->Update();

		pip.core->SetTranslate(pos);
		pip.core->SetSize({ pipSize_, pipSize_ });
		pip.core->SetScale({ s, s });
		pip.core->SetColor({ coreColor.x, coreColor.y, coreColor.z, coreAlpha * alpha });
		pip.core->Update();

		float burstAlpha = 0.0f;
		float burstScale = 1.0f;
		if (pip.burstTimer > 0.0f) {
			pip.burstTimer = (std::max)(0.0f, pip.burstTimer - deltaTime);
			float p = 1.0f - pip.burstTimer / kBurstDuration;
			burstScale = 1.0f + p * (pip.burstIsGain ? 2.0f : 2.8f);
			burstAlpha = (1.0f - p) * 0.8f;
		}
		Vector3 burstColor = pip.burstIsGain ? kReadyColor : Lerp3(theme, { 1.0f, 1.0f, 1.0f }, 0.5f);
		pip.burst->SetTranslate(pos);
		pip.burst->SetSize({ pipSize_, pipSize_ });
		pip.burst->SetScale({ groupScale * burstScale, groupScale * burstScale });
		pip.burst->SetColor({ burstColor.x, burstColor.y, burstColor.z, burstAlpha * alpha });
		pip.burst->Update();
	}

	// --- ラベル ---
	std::wstringstream label;
	const float rechargeDuration = player->GetAnchorRechargeDuration();
	const float remaining = (std::max)(0.0f, rechargeDuration - player->GetAnchorRechargeTimer());
	if (!hasStock) {
		label << L"EMPTY " << std::fixed << std::setprecision(1) << remaining << L"s";
	} else if (player->HasAnchor()) {
		label << L"DEPLOYED  x" << stock;
	} else {
		label << L"READY  x" << stock;
	}
	SetLabel(label.str());
	labelSprite_->SetTranslate({ cx, cy - 22.0f * groupScale });
	labelSprite_->SetScale({ groupScale, groupScale });
	Vector3 labelColor = Lerp3(theme, { 1.0f, 1.0f, 1.0f }, 0.2f);
	labelSprite_->SetColor({ labelColor.x, labelColor.y, labelColor.z, alpha });
	labelSprite_->Update();

	// --- リチャージバー（満タンでなければ表示） ---
	const bool showBar = stock < maxStock && rechargeDuration > 0.0f;
	const float barAlpha = showBar ? alpha : 0.0f;
	const float rate = showBar ? std::clamp(player->GetAnchorRechargeTimer() / rechargeDuration, 0.0f, 1.0f) : 0.0f;
	const float barW = barWidth_ * groupScale;
	const float barH = barHeight_ * groupScale;
	const Vector2 barPos = { cx - barW * 0.5f, cy + 16.0f * groupScale };

	barBgSprite_->SetTranslate(barPos);
	barBgSprite_->SetSize({ barW, barH });
	barBgSprite_->SetColor({ 0.05f, 0.08f, 0.12f, 0.75f * barAlpha });
	barBgSprite_->Update();

	barFillSprite_->SetTranslate(barPos);
	barFillSprite_->SetSize({ barW * rate, barH });
	barFillSprite_->SetColor({ theme.x, theme.y, theme.z, barAlpha });
	barFillSprite_->Update();
}

void AnchorStockIndicator::Draw() {
	if (!isVisible_ || !enabled_) {
		return;
	}
	for (auto& pip : pips_) {
		pip.glow->Draw();
	}
	for (auto& pip : pips_) {
		pip.burst->Draw();
		pip.core->Draw();
	}
	barBgSprite_->Draw();
	barFillSprite_->Draw();
	labelSprite_->Draw();
}

void AnchorStockIndicator::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Anchor Stock Indicator")) {
		ImGui::Checkbox("Enabled", &enabled_);
		ImGui::DragFloat("World Offset Y", &worldOffsetY_, 0.05f, 0.0f, 5.0f);
		ImGui::DragFloat("Pip Spacing", &pipSpacing_, 0.5f, 8.0f, 80.0f);
		ImGui::DragFloat("Pip Size", &pipSize_, 0.5f, 4.0f, 64.0f);
		ImGui::DragFloat("Glow Size", &glowSize_, 0.5f, 4.0f, 128.0f);
		ImGui::DragFloat("Hold Duration", &holdDuration_, 0.05f, 0.1f, 5.0f);
		ImGui::DragFloat("Fade Duration", &fadeDuration_, 0.01f, 0.05f, 2.0f);
		ImGui::DragFloat("Pop In Duration", &popInDuration_, 0.01f, 0.05f, 1.0f);
		ImGui::DragFloat("Bar Width", &barWidth_, 0.5f, 10.0f, 200.0f);
		ImGui::DragFloat("Bar Height", &barHeight_, 0.1f, 1.0f, 20.0f);
		ImGui::TreePop();
	}
#endif
}

}
