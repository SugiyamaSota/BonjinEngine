#pragma once
#include "PlayerStatusRepository.h"

/// <summary>
/// プレイヤー個体のHP・レベル・経験値を管理するクラス
/// </summary>
class PlayerStatusComponent {
public:
    void Initialize();

    // 経験値獲得 (レベルアップしたら true を返す)
    bool GainExp(int amount);

    // ダメージ処理
    void ApplyDamage(int damage);

    // ゲッター / セッター
    int GetHp() const { return hp_; }
    int GetMaxHp() const { return currentData_.maxHp; }
    int GetAttackPower() const { return currentData_.attackPower; }
    int GetLevel() const { return level_; }
    int GetExp() const { return exp_; }
    int GetRequiredExp() const { return currentData_.requiredExp; }
    float GetAnchorRechargeTime() const { return currentData_.anchorRechargeTime; }
    bool IsDead() const { return hp_ <= 0; }

    void SetHp(int hp) { hp_ = hp; }

private:
    void UpdateStatusFromRepository();

    int level_ = 1;
    int exp_ = 0;
    int hp_ = 3;
    PlayerStatusData currentData_;
};