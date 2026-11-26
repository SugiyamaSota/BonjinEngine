#pragma once
#include "PlayerCommand.h"

namespace Bonjin {
class BattleController;
}

/// <summary>
/// プレイヤーへの命令を出す側 (人の入力・デモのAI など)
/// </summary>
class IPlayerController {
public:
	virtual ~IPlayerController() = default;

	/// <summary>
	/// このフレームの命令を commands に積む
	/// </summary>
	virtual void Update(const Bonjin::BattleController& battleController, float deltaTime, PlayerCommandList& commands) = 0;
};

/// <summary>
/// キーボードとゲームパッドの入力を命令に変える
/// (どのボタンが何の命令かは、ここだけが知っている)
/// </summary>
class HumanPlayerController : public IPlayerController {
public:
	void Update(const Bonjin::BattleController& battleController, float deltaTime, PlayerCommandList& commands) override;

private:
	static inline const long kMoveDeadZone = 750; // 移動のスティック入力のデッドゾーン
	static inline const long kAimDeadZone = 750;  // アンカーの向きのスティック入力のデッドゾーン
};
