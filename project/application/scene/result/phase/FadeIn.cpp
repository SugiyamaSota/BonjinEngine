#include "FadeIn.h"
#include "Active.h"
#include "../ResultScene.h"

namespace Bonjin::ResultPhase {

void FadeIn::Update(ResultScene* scene, float deltaTime) {
	(void)deltaTime;
	scene->ChangePhase(std::make_unique<Active>());
}

}
