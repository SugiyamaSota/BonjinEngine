#pragma once

#include "../interface/BaseScene.h"
#include "../game/BattleController.h"

namespace Bonjin {

enum class TutorialPhase {
	kStart,
	kPlay,
	kComplete,
};

class TutorialScene : public BaseScene<TutorialPhase> {
public:
	void Initialize(Camera* camera) override;
	void Unload() override;
	void Update(float deltaTime) override;
	void Draw() override;
	SceneType GetNextScene() const override;
	const char* GetScenename() const override { return "TutorialScene"; }

private:
	std::unique_ptr<BattleController> battleController_;
};

}
