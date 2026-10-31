#include "Play.h"
#include "Complete.h"
#include "../TutorialScene.h"

#include "SceneManager.h"

namespace Bonjin::TutorialPhase {

void Play::Update(TutorialScene* scene, float deltaTime) {
	(void)deltaTime;
	auto tm = scene->GetTutorialManager();
	if (tm && tm->IsCompleted()) {
		scene->ChangePhase(std::make_unique<Complete>());
		SceneManager::GetInstance()->RequestSceneChange("GameScene");
	}
}

}
