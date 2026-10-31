#pragma once

#include "ITutorialPhase.h"

namespace Bonjin::TutorialPhase {

/// <summary>
/// チュートリアル完了後の遷移待ちフェーズ
/// </summary>
class Complete : public ITutorialPhase {
public:
	void Update(TutorialScene* scene, float deltaTime) override;
};

}
