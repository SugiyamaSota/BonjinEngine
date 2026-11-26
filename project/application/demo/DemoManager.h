#pragma once

namespace Bonjin {

class BattleController;
class BaseEnemy;

/// <summary>
/// 勝手にループするデモ (展示用)
/// デモモードの間は「タイトル → 一定時間で自動でゲームへ → プレイヤーが自動でゴールまで進む → クリア演出 → タイトル」をくり返す
/// </summary>
class DemoManager {
public:
	static DemoManager* GetInstance();

	// デモモードのON/OFF (ImGuiで切り替える。起動時の値は kDefaultDemoMode)
	void SetDemoMode(bool enabled);
	bool IsDemoMode() const { return isDemoMode_; }

	// タイトルで何秒待ってからゲームへ進むか
	float GetTitleWaitTime() const { return titleWaitTime_; }

	// --- ゲーム中の自動操作 ---
	// ゲーム開始時に呼ぶ (デモモードなら本物の入力を止めて自動操作に切り替える)
	void BeginGame();
	// ゲーム終了時に呼ぶ (本物の入力に戻す)
	void EndGame();
	/// <summary>
	/// プレイ中、毎フレームのゲーム更新の前に呼ぶ
	/// ゴールの方向へ歩くキーを押し、詰まったらジャンプする
	/// 目の前に敵がいたら立ち止まり、アンカーを当ててロックオンしてから撃破する
	/// </summary>
	void UpdateAutoPlay(const BattleController* battleController, float deltaTime);

	void DrawImGui();

private:
	DemoManager() = default;
	DemoManager(const DemoManager&) = delete;
	DemoManager& operator=(const DemoManager&) = delete;

	/// <summary>
	/// 目の前の敵への攻撃 (アンカー発射・撃破・外れたアンカーの回収)
	/// </summary>
	/// <returns>攻撃のために立ち止まるべきなら true</returns>
	bool UpdateAutoAttack(const BattleController* battleController, float deltaTime);
	// 今アンカーをまっすぐ撃てば当たる、一番近い敵 (いなければ nullptr)
	const BaseEnemy* FindEnemyInFront(const BattleController* battleController) const;

	// 起動したときにデモモードにするか (展示ではtrueにする)
	static inline const bool kDefaultDemoMode = true;

	bool isDemoMode_ = kDefaultDemoMode;
	bool isInGame_ = false;
	float titleWaitTime_ = 5.0f;

	// --- 自動操作のパラメータ ---
	float arriveDistance_ = 0.3f;      // ゴールとの横の距離がこれ以下なら歩くのをやめる
	float stuckSpeed_ = 0.02f;         // 歩こうとしてるのに横の速さ(1フレームの移動量)がこれ以下なら詰まっているとみなす
	float stuckTime_ = 0.15f;          // 詰まってからジャンプするまでの時間 (秒)
	float jumpHeightThreshold_ = 1.5f; // ゴールがこれより高い所にあって近いならジャンプする
	float jumpNearDistance_ = 4.0f;    // 上の「近い」の横の距離
	float jumpHoldTime_ = 0.25f;       // ジャンプキーを押し続ける時間 (秒)
	float jumpCooldown_ = 0.4f;        // 次のジャンプまで空ける時間 (秒)

	// --- 自動攻撃のパラメータ ---
	float attackRange_ = 8.0f;         // 前方のこの距離以内にいる敵を攻撃する (敵の索敵範囲より長くして先に撃つ)
	float attackHeightMargin_ = 0.2f;  // アンカーと敵の高さがこれ以上重なっていれば狙う
	float shootInterval_ = 0.3f;       // 次のアンカーを撃つまで空ける時間 (秒)
	float defeatDelay_ = 0.4f;         // ロックオンしてから撃破するまで見せる時間 (秒)

	// ゴールできずに詰まったときの保険。この秒数たったらタイトルへ戻る
	float maxPlayTime_ = 60.0f;
	float playTimer_ = 0.0f;
	bool isGiveUpRequested_ = false;

	float stuckTimer_ = 0.0f;
	float jumpHoldTimer_ = 0.0f;
	float jumpCooldownTimer_ = 0.0f;
	float shootCooldownTimer_ = 0.0f;
	float lockOnTimer_ = 0.0f; // ロックオンしてからの時間
};

}
