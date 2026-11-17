#include "FadeIn.h"
#include "Active.h"
#include "../ResultScene.h"

#include "SceneManager.h"

namespace Bonjin::ResultPhase {

void FadeIn::Update(ResultScene* scene, float deltaTime) {
	(void)deltaTime;
	// 入場トランジションが終わるまでは入力を受け付けない
	// （演出中に遷移リクエストすると SceneManager 側で無視され、FadeOut で詰まるため）
	if (SceneManager::GetInstance()->IsTransitionActive()) {
		return;
	}
	scene->ChangePhase(std::make_unique<Active>());
}

}
