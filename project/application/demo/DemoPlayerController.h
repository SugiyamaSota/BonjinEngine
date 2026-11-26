#pragma once
#include "gameObject/player/PlayerController.h"

namespace Bonjin {

class BaseEnemy;

/// <summary>
/// デモの自動操作のパラメータ (DemoManager の ImGui で調整する)
/// </summary>
struct DemoAutoPlaySettings {
	// --- 移動 ---
	float arriveDistance = 0.3f;      // ゴールとの横の距離がこれ以下なら歩くのをやめる
	float stuckSpeed = 0.02f;         // 歩こうとしてるのに横の速さ(1フレームの移動量)がこれ以下なら詰まっているとみなす
	float stuckTime = 0.15f;          // 詰まってからジャンプするまでの時間 (秒)
	float jumpHeightThreshold = 1.5f; // ゴールがこれより高い所にあって近いならジャンプする
	float jumpNearDistance = 4.0f;    // 上の「近い」の横の距離
	float jumpHoldTime = 0.25f;       // ジャンプしてから次のジャンプを考え始めるまでの時間 (秒)
	float jumpCooldown = 0.4f;        // さらに次のジャンプまで空ける時間 (秒)

	// --- 攻撃 ---
	float attackRange = 8.0f;         // 前方のこの距離以内にいる敵を攻撃する (敵の索敵範囲より長くして先に撃つ)
	float attackHeightMargin = 0.2f;  // アンカーと敵の高さがこれ以上重なっていれば狙う
	float shootInterval = 0.3f;       // 次のアンカーを撃つまで空ける時間 (秒)
	float defeatDelay = 0.4f;         // ロックオンしてから撃破するまで見せる時間 (秒)
};

/// <summary>
/// デモ用のAI。人の入力の代わりに、同じ命令 (IPlayerCommand) をプレイヤーに出す
/// ゴールの方へ歩き、詰まったらジャンプし、目の前に敵がいたら立ち止まってアンカーで倒す
/// </summary>
class DemoPlayerController : public IPlayerController {
public:
	explicit DemoPlayerController(const DemoAutoPlaySettings& settings) : settings_(settings) {}

	void Update(const BattleController& battleController, float deltaTime, PlayerCommandList& commands) override;

private:
	/// <summary>
	/// 目の前の敵への攻撃 (アンカー発射・撃破・外れたアンカーの回収)
	/// </summary>
	/// <returns>攻撃のために立ち止まるべきなら true</returns>
	bool UpdateAttack(const BattleController& battleController, float deltaTime, PlayerCommandList& commands);
	// 今アンカーをまっすぐ撃てば当たる、一番近い敵 (いなければ nullptr)
	const BaseEnemy* FindEnemyInFront(const BattleController& battleController) const;

	const DemoAutoPlaySettings& settings_;

	// デモモードをOFFにしたら、その場で人の操作に戻す
	HumanPlayerController human_;

	float stuckTimer_ = 0.0f;
	float jumpHoldTimer_ = 0.0f;
	float jumpCooldownTimer_ = 0.0f;
	float shootCooldownTimer_ = 0.0f;
	float lockOnTimer_ = 0.0f;       // ロックオンしてからの時間
	bool wasAnchorHeld_ = false;     // 前のフレームにアンカーのボタンを押していたか (押した瞬間を作る用)
};

}
