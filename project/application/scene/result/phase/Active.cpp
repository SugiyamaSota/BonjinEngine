#include "Active.h"
#include "FadeOut.h"
#include "../ResultScene.h"

#include "input/Input.h"
#include "input/Gamepad.h"
#include "SceneManager.h"

namespace Bonjin::ResultPhase {

void Active::Update(ResultScene* scene, float deltaTime) {
	(void)deltaTime;
	if (Input::GetInstance()->IsTrigger(DIK_SPACE) || Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A)) {
		scene->ChangePhase(std::make_unique<FadeOut>());
		SceneManager::GetInstance()->RequestSceneChange("TitleScene");
	}
}

}
