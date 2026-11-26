#pragma once
#include "../../logic/Data.h"
#include <memory>
#include <vector>

class Player;

/// <summary>
/// アンカーを撃つ向き
/// </summary>
enum class AnchorAim {
	kStraight, // 水平
	kUp,       // 斜め上 (Wキー / スティック下側)
	kDown,     // 斜め下 (Sキー / スティック上側)
};

/// <summary>
/// プレイヤーへの命令 (Command)
/// 誰が出した命令か (キーボード・ゲームパッド・デモのAI) を Player は気にしない
/// </summary>
class IPlayerCommand {
public:
	virtual ~IPlayerCommand() = default;

	// 命令をプレイヤーに実行させる
	virtual void Execute(Player& player) const = 0;
};

// 1フレーム分の命令の列
using PlayerCommandList = std::vector<std::unique_ptr<IPlayerCommand>>;

/// <summary>
/// 左右に移動する (押している間、毎フレーム出す)
/// </summary>
class MoveCommand : public IPlayerCommand {
public:
	explicit MoveCommand(LRDirection direction) : direction_(direction) {}
	void Execute(Player& player) const override;

private:
	LRDirection direction_;
};

/// <summary>
/// ジャンプする (地上にいるときだけ跳ぶ)
/// </summary>
class JumpCommand : public IPlayerCommand {
public:
	void Execute(Player& player) const override;
};

/// <summary>
/// アンカーのボタンを押した瞬間 (アンカーが無ければ発射)
/// </summary>
class TriggerAnchorCommand : public IPlayerCommand {
public:
	void Execute(Player& player) const override;
};

/// <summary>
/// アンカーのボタンを押し続けている (刺さったアンカーを長押しで回収)
/// </summary>
class HoldAnchorCommand : public IPlayerCommand {
public:
	void Execute(Player& player) const override;
};

/// <summary>
/// アンカーを撃つ向きを決める
/// </summary>
class AimAnchorCommand : public IPlayerCommand {
public:
	explicit AimAnchorCommand(AnchorAim aim) : aim_(aim) {}
	void Execute(Player& player) const override;

private:
	AnchorAim aim_;
};

/// <summary>
/// 刺さったアンカーの場所へテレポートする
/// </summary>
class TeleportCommand : public IPlayerCommand {
public:
	void Execute(Player& player) const override;
};

/// <summary>
/// ロックオン中の敵をまとめて倒す
/// </summary>
class DefeatLockedOnCommand : public IPlayerCommand {
public:
	void Execute(Player& player) const override;
};
