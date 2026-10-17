#define NOMINMAX
#include "HUD.h"
#include "../gameObject/player/Player.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifdef USE_IMGUI
#include "ImGuiManager.h"
#endif

namespace Bonjin {

HUD::HUD() = default;
HUD::~HUD() = default;

void HUD::Initialize() {
	// 1. 背景パネル
	bgPanelSprite_ = std::make_unique<Sprite>();
	bgPanelSprite_->Initialize("default.png");
	bgPanelSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });
	bgPanelSprite_->SetSize(panelSize_);
	bgPanelSprite_->SetTranslate(basePosition_);
	bgPanelSprite_->SetColor({ 0.05f, 0.08f, 0.14f, 0.75f });

	// 2. HPバー背景
	hpBarBgSprite_ = std::make_unique<Sprite>();
	hpBarBgSprite_->Initialize("default.png");
	hpBarBgSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });
	hpBarBgSprite_->SetSize(hpBarSize_);
	hpBarBgSprite_->SetTranslate({ basePosition_.x + 15.0f, basePosition_.y + 46.0f });
	hpBarBgSprite_->SetColor({ 0.12f, 0.12f, 0.16f, 0.90f });

	// 3. HPバー ダメージ追従ゲージ
	hpBarDamageSprite_ = std::make_unique<Sprite>();
	hpBarDamageSprite_->Initialize("default.png");
	hpBarDamageSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });
	hpBarDamageSprite_->SetSize(hpBarSize_);
	hpBarDamageSprite_->SetTranslate({ basePosition_.x + 15.0f, basePosition_.y + 46.0f });
	hpBarDamageSprite_->SetColor({ 0.90f, 0.30f, 0.20f, 0.85f });

	// 4. HPバー 実値ゲージ
	hpBarFillSprite_ = std::make_unique<Sprite>();
	hpBarFillSprite_->Initialize("default.png");
	hpBarFillSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });
	hpBarFillSprite_->SetSize(hpBarSize_);
	hpBarFillSprite_->SetTranslate({ basePosition_.x + 15.0f, basePosition_.y + 46.0f });
	hpBarFillSprite_->SetColor({ 0.20f, 0.85f, 0.45f, 1.0f });

	// 5. テキストスプライト初期化
	levelTextSprite_ = std::make_unique<TextSprite>();
	levelTextSprite_->Initialize();
	levelTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	hpTextSprite_ = std::make_unique<TextSprite>();
	hpTextSprite_->Initialize();
	hpTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	statusTextSprite_ = std::make_unique<TextSprite>();
	statusTextSprite_->Initialize();
	statusTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	anchorTextSprite_ = std::make_unique<TextSprite>();
	anchorTextSprite_->Initialize();
	anchorTextSprite_->SetAnchor({ 0.0f, 0.0f, 0.0f });

	// 初期表示テキスト設定
	UpdateTextSprites(1, 3, 3, 1, 0, 100);
}

void HUD::UpdateTextSprites(int level, int hp, int maxHp, int atk, int exp, int reqExp) {
	// レベルテキスト
	std::wstringstream lvStream;
	lvStream << L"LV. " << level;
	levelTextSprite_->SetText(lvStream.str(), 20, RGB(255, 215, 60)); // ゴールド色
	levelTextSprite_->SetTranslate({ basePosition_.x + 15.0f, basePosition_.y + 12.0f });

	// HPテキスト
	std::wstringstream hpStream;
	hpStream << L"HP " << hp << L" / " << maxHp;
	COLORREF hpColor = RGB(240, 240, 240);
	if (maxHp > 0 && (float)hp / maxHp <= 0.3f) {
		hpColor = RGB(255, 80, 80);
	}
	hpTextSprite_->SetText(hpStream.str(), 18, hpColor);
	hpTextSprite_->SetTranslate({ basePosition_.x + 120.0f, basePosition_.y + 14.0f });

	// ステータステキスト (ATK / EXP)
	std::wstringstream statusStream;
	statusStream << L"ATK: " << atk << L"    EXP: " << exp << L" / " << reqExp;
	statusTextSprite_->SetText(statusStream.str(), 18, RGB(200, 230, 255)); // 明るいアイスブルー
	statusTextSprite_->SetTranslate({ basePosition_.x + 15.0f, basePosition_.y + 75.0f });

	// キャッシュ更新
	cachedLevel_ = level;
	cachedHp_ = hp;
	cachedMaxHp_ = maxHp;
	cachedAttack_ = atk;
	cachedExp_ = exp;
	cachedRequiredExp_ = reqExp;
}

void HUD::Update(const Player* player, float deltaTime) {
	if (!player || !isVisible_) return;

	int level = player->GetLevel();
	int hp = player->GetHp();
	int maxHp = player->GetMaxHp();
	int atk = player->GetAttackPower();
	int exp = player->GetExp();
	int reqExp = player->GetRequiredExp();

	// 初回または変更検知時にテキストを更新
	if (level != cachedLevel_ || hp != cachedHp_ || maxHp != cachedMaxHp_ ||
		atk != cachedAttack_ || exp != cachedExp_ || reqExp != cachedRequiredExp_) {
		UpdateTextSprites(level, hp, maxHp, atk, exp, reqExp);
	}

	// ダメージバーの遅延追従処理
	if (displayedDamageHp_ > static_cast<float>(hp)) {
		displayedDamageHp_ -= deltaTime * (std::max)(1.0f, static_cast<float>(maxHp) * 0.8f);
		if (displayedDamageHp_ < static_cast<float>(hp)) {
			displayedDamageHp_ = static_cast<float>(hp);
		}
	} else {
		displayedDamageHp_ = static_cast<float>(hp);
	}

	// HPバーの割合計算
	float maxHpFloat = static_cast<float>((std::max)(1, maxHp));
	float hpRate = (std::clamp)(static_cast<float>(hp) / maxHpFloat, 0.0f, 1.0f);
	float damageRate = (std::clamp)(displayedDamageHp_ / maxHpFloat, 0.0f, 1.0f);

	// スプライトの位置とサイズ更新
	bgPanelSprite_->SetTranslate(basePosition_);
	bgPanelSprite_->SetSize(panelSize_);

	const float barX = basePosition_.x + 15.0f;
	const float barY = basePosition_.y + 44.0f;

	hpBarBgSprite_->SetTranslate({ barX, barY });
	hpBarBgSprite_->SetSize(hpBarSize_);

	hpBarDamageSprite_->SetTranslate({ barX, barY });
	hpBarDamageSprite_->SetSize({ hpBarSize_.x * damageRate, hpBarSize_.y });

	hpBarFillSprite_->SetTranslate({ barX, barY });
	hpBarFillSprite_->SetSize({ hpBarSize_.x * hpRate, hpBarSize_.y });

	// HP割合に応じたバーカラーの調整
	if (hpRate > 0.5f) {
		hpBarFillSprite_->SetColor({ 0.20f, 0.85f, 0.45f, 1.0f }); // グリーン
	} else if (hpRate > 0.25f) {
		hpBarFillSprite_->SetColor({ 0.95f, 0.80f, 0.20f, 1.0f }); // イエロー
	} else {
		hpBarFillSprite_->SetColor({ 0.95f, 0.25f, 0.25f, 1.0f }); // レッド
	}

	// アンカーストックとリチャージテキスト更新
	int stock = player->GetAnchorStock();
	int maxStock = player->GetMaxAnchorStock();
	float rechargeTimer = player->GetAnchorRechargeTimer();
	float rechargeDuration = player->GetAnchorRechargeDuration();

	std::wstringstream anchorStream;
	anchorStream << L"ANCHOR: ";
	for (int i = 0; i < maxStock; ++i) {
		if (i < stock) {
			anchorStream << L"◆ ";
		} else {
			anchorStream << L"◇ ";
		}
	}
	if (stock < maxStock && rechargeDuration > 0.0f) {
		float remaining = (std::max)(0.0f, rechargeDuration - rechargeTimer);
		anchorStream << L"(" << std::fixed << std::setprecision(1) << remaining << L"s)";
	}
	COLORREF anchorColor = (stock > 0) ? RGB(100, 220, 255) : RGB(255, 100, 100);
	anchorTextSprite_->SetText(anchorStream.str(), 18, anchorColor);
	anchorTextSprite_->SetTranslate({ basePosition_.x + 15.0f, basePosition_.y + 98.0f });

	// 各スプライトの内部トランスフォーム更新
	bgPanelSprite_->Update();
	hpBarBgSprite_->Update();
	hpBarDamageSprite_->Update();
	hpBarFillSprite_->Update();

	levelTextSprite_->Update();
	hpTextSprite_->Update();
	statusTextSprite_->Update();
	anchorTextSprite_->Update();
}

void HUD::Draw() {
	if (!isVisible_) return;

	// 背景とバー
	if (bgPanelSprite_) bgPanelSprite_->Draw();
	if (hpBarBgSprite_) hpBarBgSprite_->Draw();
	if (hpBarDamageSprite_) hpBarDamageSprite_->Draw();
	if (hpBarFillSprite_) hpBarFillSprite_->Draw();

	// テキスト
	if (levelTextSprite_) levelTextSprite_->Draw();
	if (hpTextSprite_) hpTextSprite_->Draw();
	if (statusTextSprite_) statusTextSprite_->Draw();
	if (anchorTextSprite_) anchorTextSprite_->Draw();
}

void HUD::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("HUD Debug")) {
		ImGui::Checkbox("Visible", &isVisible_);
		ImGui::DragFloat2("Base Position", &basePosition_.x, 1.0f, 0.0f, 1280.0f);
		ImGui::DragFloat2("Panel Size", &panelSize_.x, 1.0f, 50.0f, 800.0f);
		ImGui::DragFloat2("HP Bar Size", &hpBarSize_.x, 1.0f, 50.0f, 800.0f);
		ImGui::TreePop();
	}
#endif
}

}
