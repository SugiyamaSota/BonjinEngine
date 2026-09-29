#include "PlayerStatusComponent.h"

void PlayerStatusComponent::Initialize() {
    level_ = 1;
    exp_ = 0;
    UpdateStatusFromRepository();
    hp_ = currentData_.maxHp;
}

bool PlayerStatusComponent::GainExp(int amount) {
    if (IsDead()) return false;

    exp_ += amount;
    bool levelUp = false;

    while (exp_ >= GetRequiredExp() && GetRequiredExp() > 0) {
        exp_ -= GetRequiredExp();
        level_++;
        levelUp = true;
    }

    if (levelUp) {
        UpdateStatusFromRepository();
        hp_ = currentData_.maxHp; // レベルアップ時全回復
    }

    return levelUp;
}

void PlayerStatusComponent::ApplyDamage(int damage) {
    hp_ = (hp_ > damage) ? hp_ - damage : 0;
}

void PlayerStatusComponent::UpdateStatusFromRepository() {
    currentData_ = PlayerStatusRepository::GetInstance()->GetStatusByLevel(level_);
}