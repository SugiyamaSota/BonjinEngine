#include "PlayerCommand.h"
#include "Player.h"

void MoveCommand::Execute(Player& player) const { player.RequestMove(direction_); }
void JumpCommand::Execute(Player& player) const { player.RequestJump(); }
void TriggerAnchorCommand::Execute(Player& player) const { player.RequestAnchorTrigger(); }
void HoldAnchorCommand::Execute(Player& player) const { player.RequestAnchorHold(); }
void AimAnchorCommand::Execute(Player& player) const { player.RequestAnchorAim(aim_); }
void TeleportCommand::Execute(Player& player) const { player.RequestTeleport(); }
void DefeatLockedOnCommand::Execute(Player& player) const { player.RequestDefeatLockedOn(); }
