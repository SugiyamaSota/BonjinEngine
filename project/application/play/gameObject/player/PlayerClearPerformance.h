#pragma once
#include "PlayerAnimator.h"
#include <memory>

/// <summary>
/// クリア演出の段階 (どの段階かを外から知るための名前。振る舞いは各段階のクラスが持つ)
/// </summary>
enum class PlayerClearPhase {
	kNone,     // 演出していない
	kLanding,  // 着地待ち
	kStart,    // Flair開始
	kLoop,     // Flairループ
	kFinish,   // Flair終了
	kVanish,   // 決めポーズ後、ふくらんでから縮んで消える
	kDone,     // 演出終了 (最後のポーズで待機)
};

/// <summary>
/// クリア演出の調整用パラメータ
/// </summary>
struct ClearPerformanceSettings {
	int flairLoopCount = 2;           // Flairのループ回数
	float landingTimeout = 1.0f;      // 着地待ちの最大時間 (落ち続けても演出を始める)
	float poseHoldTime = 0.35f;       // 決めポーズのまま止まる時間
	float vanishGrowTime = 0.12f;     // ふくらむ時間
	float vanishGrowScale = 1.2f;     // ふくらむ大きさ
	float vanishShrinkTime = 0.18f;   // 縮んで消えるまでの時間
	bool vanishLightning = true;      // 消えた瞬間に雷を出す
	float vanishLightningHeight = 20.0f; // 雷を上へ伸ばす長さ
};

/// <summary>
/// 毎フレーム Player から受け取る情報
/// </summary>
struct ClearPerformanceInput {
	bool isOnGround = false;
	float flairStartLength = 0.0f;   // Flair開始アニメーションの長さ(秒)
	float flairingLength = 0.0f;     // Flairループ1回の長さ(秒)
	float flairFinishLength = 0.0f;  // Flair終了アニメーションの長さ(秒)
};

class PlayerClearPerformance;

/// <summary>
/// クリア演出の1段階 (State)
/// </summary>
class IClearState {
public:
	virtual ~IClearState() = default;

	// 今の段階の名前
	virtual PlayerClearPhase GetPhase() const = 0;

	/// <summary>
	/// 1フレーム進める
	/// </summary>
	/// <returns>次の段階へ進むならその段階 (このままなら nullptr)</returns>
	virtual std::unique_ptr<IClearState> Update(PlayerClearPerformance& performance) = 0;

	// 再生するアニメーション
	virtual PlayerAnimator::State GetAnimation(const PlayerClearPerformance& performance) const = 0;

	// カメラの寄り具合 (0:ふだんの距離 1:寄りの距離)
	virtual float GetCameraZoom(const PlayerClearPerformance& performance) const = 0;

	// ゴールのポータルを雷の色へ近づける割合 (loopProgress: Flairループで寄った回数の進み具合 0~1)
	virtual float GetLightningBlend(float loopProgress) const = 0;
};

/// <summary>
/// プレイヤーのクリア演出 (着地待ち -> Flair開始 -> ループ -> 終了 -> 消える)
/// 段階の切り替えと、段階をまたいで使う値 (経過時間・見た目の大きさ) を持つ
/// </summary>
class PlayerClearPerformance {
public:
	PlayerClearPerformance();
	~PlayerClearPerformance();

	// 演出を始める (着地待ちから)
	void Start();
	// 演出をやめる (演出していない状態に戻す)
	void Stop();
	// 演出を最後まで飛ばす (アニメーションが無いとき用)
	void Skip();

	// 1フレーム進める
	void Update(const ClearPerformanceInput& input, float deltaTime);

	bool IsActive() const { return state_ != nullptr; }
	bool IsFinished() const { return GetPhase() == PlayerClearPhase::kDone; }
	PlayerClearPhase GetPhase() const;
	// 今の段階に入ってからの経過時間(秒)
	float GetTimer() const { return timer_; }

	PlayerAnimator::State GetAnimation() const;
	float GetCameraZoom() const;
	float GetLightningBlend(float loopProgress) const;
	float GetVisualScale() const { return visualScale_; }

	// 消えた瞬間に雷を出すタイミングか (一度だけ true を返す)
	bool ConsumeVanishLightningRequest();

	ClearPerformanceSettings& GetSettings() { return settings_; }
	const ClearPerformanceSettings& GetSettings() const { return settings_; }

	// --- 各段階から使う ---
	const ClearPerformanceInput& GetInput() const { return input_; }
	void SetVisualScale(float scale) { visualScale_ = scale; }
	void RequestVanishLightning() { vanishLightningRequested_ = true; }

private:
	void ChangeState(std::unique_ptr<IClearState> next);

	std::unique_ptr<IClearState> state_; // nullptr のときは演出していない
	ClearPerformanceSettings settings_;
	ClearPerformanceInput input_;
	float timer_ = 0.0f;
	float visualScale_ = 1.0f;
	bool vanishLightningRequested_ = false;
};
