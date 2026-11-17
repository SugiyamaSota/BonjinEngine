#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Sprite.h"
#include "TextSprite.h"
#include "Vector.h"

class Camera;
class Player;

namespace Bonjin {

/// <summary>
/// アンカー操作（Jキー / Xボタン）時にプレイヤー頭上へポップアップ表示する
/// アンカーストック数・使用可否インジケーター
/// ストックあり: グリーン系 / ストック0: レッド系（拒否時はシェイク）
/// </summary>
class AnchorStockIndicator {
public:
	AnchorStockIndicator();
	~AnchorStockIndicator();

	void Initialize();

	/// <summary>
	/// 更新処理（入力要求の受け取り、アニメーション、画面座標追従）
	/// </summary>
	void Update(Player* player, Camera* camera, float deltaTime);

	void Draw();

	void DrawImGui();

private:
	struct Pip {
		std::unique_ptr<Sprite> glow;   // 背面の柔らかい発光
		std::unique_ptr<Sprite> core;   // 本体の玉
		std::unique_ptr<Sprite> burst;  // 消費/回復時に広がるリング風の光
		float burstTimer = 0.0f;        // >0 の間バースト演出
		bool burstIsGain = false;       // 回復(true) / 消費(false)
		float popTimer = 0.0f;          // 回復時のポップ
	};

	std::vector<Pip> pips_;
	std::unique_ptr<Sprite> barBgSprite_;
	std::unique_ptr<Sprite> barFillSprite_;
	std::unique_ptr<TextSprite> labelSprite_;

	// 表示状態
	bool isVisible_ = false;
	float showTimer_ = 0.0f;     // 表示開始からの経過
	float holdRemaining_ = 0.0f; // 残り表示時間（フェード前）
	float fadeTimer_ = 0.0f;     // フェードアウト経過
	float shakeTimer_ = 0.0f;    // 拒否シェイク
	float deniedFlash_ = 0.0f;   // 拒否時の赤フラッシュ
	float bumpTimer_ = 0.0f;     // 表示中の再入力時の軽い拡大
	int prevStock_ = -1;

	// 調整パラメータ
	bool enabled_ = true;
	float worldOffsetY_ = 2.75f; // プレイヤー中心からのワールド上方向オフセット (頭(+1.9)から約0.85上)
	float pipSpacing_ = 26.0f;
	float pipSize_ = 16.0f;
	float glowSize_ = 38.0f;
	float holdDuration_ = 1.3f;
	float fadeDuration_ = 0.3f;
	float popInDuration_ = 0.2f;
	float barWidth_ = 70.0f;
	float barHeight_ = 4.0f;

	std::wstring cachedLabel_;

	void EnsurePipCount(int count);
	void SetLabel(const std::wstring& text);
};

}
