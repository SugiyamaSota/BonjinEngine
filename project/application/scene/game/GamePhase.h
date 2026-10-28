#pragma once

#include "../../engine/interface/BaseScene.h"
#include "PostEffectTypes.h"
#include "Sprite.h"
#include <vector>
#include <memory>
#include <random>

namespace Bonjin {

class GameScene;

/// <summary>
/// GameSceneのフェーズ基底インターフェース
/// </summary>
using IGamePhase = IScenePhase<GameScene>;

/// <summary>
/// ハイライト演出フェーズ（カメラパン、実体化演出など）
/// </summary>
class GamePhaseHighlight : public IGamePhase {
public:
	enum class SubState {
		kFadeIn,
		kCameraPan,
		kFadeOut,
		kReturnToPlayer,
		kPlayerFocusIn
	};

	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void DrawImGui(GameScene* scene) override;
	void Exit(GameScene* scene) override;

	void FinishHighlight(GameScene* scene);

private:
	void SetupHighlightDissolve();

	SubState subState_ = SubState::kFadeIn;
	size_t currentHighlightIndex_ = 0;
	float timer_ = 0.0f;
	Vector3 returnStartPos_ = { 0.0f, 0.0f, 0.0f };
	DissolveMode dissolveMode_ = DissolveMode::kRightToLeft;
	static constexpr float kFadeDuration = 0.5f;
	static constexpr float kPlayerFocusDuration = 0.8f;
	static constexpr float kReturnDuration = 1.8f;
	static constexpr float kMaxPullBackRadius = 90.0f;
};

/// <summary>
/// 通常プレイフェーズ（プレイヤー操作、敵挙動、ゴール/ゲームオーバー判定）
/// </summary>
class GamePhasePlay : public IGamePhase {
public:
	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void Exit(GameScene* scene) override;
};

/// <summary>
/// ゴール/ゲームオーバー後の遷移待ちフェーズ
/// </summary>
class GamePhaseGoal : public IGamePhase {
public:
	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void Exit(GameScene* scene) override;
};

/// <summary>
/// ゲームオーバー演出フェーズ（赤いエラーウィンドウが出現して画面が暗転）
/// </summary>
class GamePhaseGameOver : public IGamePhase {
public:
	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void Draw(GameScene* scene) override;
	void Exit(GameScene* scene) override;

private:
	// エラーウィンドウ1枚分のデータ
	struct ErrorWindow {
		std::unique_ptr<Sprite> body;       // 赤い本体
		std::unique_ptr<Sprite> titleBar;   // 少し暗い赤のタイトルバー
		float spawnTime = 0.0f;             // 出現開始時刻
		float alpha = 0.0f;                 // 現在アルファ値
		float scaleAnim = 0.0f;             // ポップイン用スケール (0→1)
	};

	void SpawnWindow();
	void BuildSpawnList(); // グリッドセルをシャッフルしてスポーンリストを構築

	std::vector<ErrorWindow> windows_;
	std::mt19937 rng_;

	// グリッド分割スポーン用
	struct SpawnCell { float x, y, w, h; }; // セルの基準矩形
	std::vector<SpawnCell> spawnList_; // シャッフル済みスポーン順序

	float timer_             = 0.0f;
	float nextSpawnTimer_    = 0.0f;
	int   spawnCount_        = 0;
	bool  sceneChangeRequested_ = false;
	uint32_t shutdownSEId_   = 0;

	// 画面解像度
	static constexpr float kScreenW = 1280.0f;
	static constexpr float kScreenH = 720.0f;

	// グリッド分割数
	static constexpr int   kGridCols          = 5;     // 横分割数
	static constexpr int   kGridRows          = 5;     // 縦分割数
	// 演出パラメータ
	static constexpr float kSpawnInterval     = 0.05f; // 出現間隔（秒）
	static constexpr float kPopInDuration     = 0.12f; // ポップイン時間
	static constexpr float kDarkenStartTime   = 0.5f;  // 暗転開始時刻
	static constexpr float kJitter            = 30.0f; // セル内位置ジッター幅（px）
	static constexpr float kShutDownDuration     = 0.24f; // CRT電源断の秒数（TitleSceneと同じ）
	static constexpr float kShutDownWaitDuration = 0.50f; // 電源断後のブラックアウト秒数
};

}
