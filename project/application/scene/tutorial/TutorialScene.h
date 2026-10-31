#pragma once

#include "../interface/BaseScene.h"
#include "battle/BattleController.h"
#include "TutorialManager.h"
#include "TutorialEditor.h"

namespace Bonjin {

class TutorialScene : public BaseScene<TutorialScene> {
public:
	void Initialize(Camera* camera) override;
	void Unload() override;
	void Update(float deltaTime) override;
	void Draw() override;
	SceneType GetNextScene() const override;
	void DrawSceneImGui() override;
	const char* GetScenename() const override { return "TutorialScene"; }

	// フェーズから参照するアクセサ
	TutorialManager* GetTutorialManager() const { return tutorialManager_.get(); }

private:
	std::unique_ptr<BattleController> battleController_;
	std::unique_ptr<TutorialManager> tutorialManager_;
	std::unique_ptr<TutorialEditor> tutorialEditor_;
};

}
