#pragma once
#include <memory>
#include <string>
#include "Sprite.h"
#include "TextSprite.h"

class Player;

namespace Bonjin {

/// <summary>
/// 画面左上に表示するHUD（レベル、HP数値、HPバー、ステータス）
/// </summary>
class HUD {
public:
	HUD();
	~HUD();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="player">プレイヤー情報</param>
	/// <param name="deltaTime">フレーム時間</param>
	void Update(const Player* player, float deltaTime);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// デバッグ用ImGui描画
	/// </summary>
	void DrawImGui();

private:
	// スプライト
	std::unique_ptr<Sprite> bgPanelSprite_;
	std::unique_ptr<Sprite> hpBarBgSprite_;
	std::unique_ptr<Sprite> hpBarDamageSprite_;
	std::unique_ptr<Sprite> hpBarFillSprite_;

	// テキストスプライト
	std::unique_ptr<TextSprite> levelTextSprite_;
	std::unique_ptr<TextSprite> hpTextSprite_;
	std::unique_ptr<TextSprite> statusTextSprite_;

	// 前フレームの状態キャッシュ（テキスト更新頻度の最適化用）
	int cachedLevel_ = -1;
	int cachedHp_ = -1;
	int cachedMaxHp_ = -1;
	int cachedAttack_ = -1;
	int cachedExp_ = -1;
	int cachedRequiredExp_ = -1;

	// ダメージバーの遅延追従アニメーション用
	float displayedDamageHp_ = 0.0f;

	// 配置・外観パラメータ
	Vector2 basePosition_ = { 20.0f, 20.0f };
	Vector2 panelSize_ = { 290.0f, 135.0f };
	Vector2 hpBarSize_ = { 250.0f, 16.0f };
	bool isVisible_ = true;

	void UpdateTextSprites(int level, int hp, int maxHp, int atk, int exp, int reqExp);
};

}
