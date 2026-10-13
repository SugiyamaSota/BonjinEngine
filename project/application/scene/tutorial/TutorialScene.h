#pragma once

#include "../interface/BaseScene.h"
#include "../game/BattleController.h"
#include "TutorialManager.h"
#include "TutorialEditor.h"

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
	void DrawSceneImGui() override;
	const char* GetScenename() const override { return "TutorialScene"; }

private:
	std::unique_ptr<BattleController> battleController_;
	std::unique_ptr<TutorialManager> tutorialManager_;
	std::unique_ptr<TutorialEditor> tutorialEditor_;

	// 開幕デジタルディゾルブ（黒からの実体化）用タイマー
	float startTransitionTimer_ = 0.0f;
	const float kStartTransitionDuration = 1.4f;
};

}
