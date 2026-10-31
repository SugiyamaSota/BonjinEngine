#pragma once

#include "ITutorialPhase.h"

namespace Bonjin::TutorialPhase {

/// <summary>
/// チュートリアル進行フェーズ（完了判定でGameSceneへ）
/// </summary>
class Play : public ITutorialPhase {
public:
	void Update(TutorialScene* scene, float deltaTime) override;
};

}
