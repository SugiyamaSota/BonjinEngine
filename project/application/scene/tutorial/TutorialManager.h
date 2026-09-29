#pragma once
#include <memory>
#include <string>
#include "Sprite.h"
#include "TextSprite.h"

namespace Bonjin {

class BattleController;

/// <summary>
/// チュートリアルの進行段階
/// </summary>
enum class TutorialStep {
	kWelcome,       // チュートリアル開始案内
	kMove,          // 左右移動
	kJump,          // ジャンプ
	kAnchor,        // アンカー発射
	kLockOn,        // 敵ロックオン
	kTeleportKill,  // 雷撃テレポート一括撃破
	kGoToGoal,      // ゴール到達
	kCompleted,     // チュートリアル完了
};

/// <summary>
/// チュートリアルの進行・ミッションUI・条件判定を管理するクラス
/// </summary>
class TutorialManager {
public:
	TutorialManager();
	~TutorialManager();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="battleController">バトルコントローラー</param>
	/// <param name="deltaTime">フレーム時間</param>
	void Update(BattleController* battleController, float deltaTime);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// デバッグ用ImGui描画
	/// </summary>
	void DrawImGui();

	/// <summary>
	/// ステップの変更
	/// </summary>
	void ChangeStep(TutorialStep nextStep);

	/// <summary>
	/// チュートリアルが完了したかどうか
	/// </summary>
	bool IsCompleted() const { return currentStep_ == TutorialStep::kCompleted; }

	/// <summary>
	/// 現在のステップ取得
	/// </summary>
	TutorialStep GetCurrentStep() const { return currentStep_; }

private:
	// トランジション状態
	enum class TransitionState {
		kNone,      // 通常表示
		kClosing,   // 高さを潰している最中 (1.0 -> 0.0)
		kOpening,   // 高さを元に戻している最中 (0.0 -> 1.0)
	};

	TutorialStep currentStep_ = TutorialStep::kWelcome;
	TutorialStep nextStep_ = TutorialStep::kWelcome;
	TransitionState transitionState_ = TransitionState::kNone;
	float transitionTimer_ = 0.0f;
	float transitionDuration_ = 0.16f; // 潰れる/戻る時間(秒)
	float currentScaleY_ = 1.0f;       // 現在の縦スケール (0.0f ~ 1.0f)

	float stepTimer_ = 0.0f;
	float moveAccumulator_ = 0.0f;
	bool hasJumped_ = false;
	bool hasShotAnchor_ = false;
	bool hasLockedOn_ = false;
	bool hasKilledEnemy_ = false;

	// UIスプライト
	std::unique_ptr<Sprite> bannerBgSprite_;
	std::unique_ptr<Sprite> bannerLineSprite_;
	std::unique_ptr<TextSprite> titleTextSprite_;
	std::unique_ptr<TextSprite> detailTextSprite_;
	std::unique_ptr<TextSprite> skipGuideTextSprite_;

	// 配置パラメータ
	Vector2 bannerPosition_ = { 340.0f, 20.0f };
	Vector2 bannerSize_ = { 600.0f, 95.0f };

	void UpdateBannerTexts();
};

}
