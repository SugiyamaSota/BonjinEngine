#include "Goal.h"
#include "../GameScene.h"

namespace Bonjin::GamePhase {

void Goal::Enter(GameScene* scene) {
	(void)scene;
}

void Goal::Update(GameScene* scene, float deltaTime) {
	(void)scene;
	(void)deltaTime;
	// リザルトシーンへの遷移待ち
}

void Goal::Exit(GameScene* scene) {
	(void)scene;
}

}
