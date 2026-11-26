#include "PlayerController.h"
#include "input/Input.h"
#include "input/Gamepad.h"

using namespace Bonjin;

void HumanPlayerController::Update(const BattleController&, float, PlayerCommandList& commands) {
	Input* input = Input::GetInstance();
	Gamepad* gamepad = Gamepad::GetInstance();
	const long stickX = gamepad->GetLStickX();
	const long stickY = gamepad->GetLStickY();

	// 移動 (押している間)
	if (stickX > kMoveDeadZone || input->IsPress(DIK_D)) {
		commands.push_back(std::make_unique<MoveCommand>(LRDirection::kRight));
	}
	if (stickX < -kMoveDeadZone || input->IsPress(DIK_A)) {
		commands.push_back(std::make_unique<MoveCommand>(LRDirection::kLeft));
	}

	// ジャンプ (パッドのAは押している間、スペースは押した瞬間)
	if (gamepad->IsPress(XINPUT_GAMEPAD_A) || input->IsTrigger(DIK_SPACE)) {
		commands.push_back(std::make_unique<JumpCommand>());
	}

	// アンカーの向き
	if (stickY > kAimDeadZone || input->IsPress(DIK_S)) {
		commands.push_back(std::make_unique<AimAnchorCommand>(AnchorAim::kDown));
	} else if (stickY < -kAimDeadZone || input->IsPress(DIK_W)) {
		commands.push_back(std::make_unique<AimAnchorCommand>(AnchorAim::kUp));
	}

	// アンカー (押した瞬間で発射、押し続けで回収)
	if (gamepad->IsTrigger(XINPUT_GAMEPAD_X) || input->IsTrigger(DIK_J)) {
		commands.push_back(std::make_unique<TriggerAnchorCommand>());
	}
	if (gamepad->IsPress(XINPUT_GAMEPAD_X) || input->IsPress(DIK_J)) {
		commands.push_back(std::make_unique<HoldAnchorCommand>());
	}

	// テレポート
	if (gamepad->IsTrigger(XINPUT_GAMEPAD_B) || input->IsTrigger(DIK_K)) {
		commands.push_back(std::make_unique<TeleportCommand>());
	}

	// ロックオン中の敵を撃破
	if (gamepad->IsTrigger(XINPUT_GAMEPAD_Y) || input->IsTrigger(DIK_L)) {
		commands.push_back(std::make_unique<DefeatLockedOnCommand>());
	}
}
