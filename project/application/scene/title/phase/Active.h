#pragma once

#include "ITitlePhase.h"

namespace Bonjin::TitlePhase {

/// <summary>
/// タイトル待機・操作フェーズ
/// デモモードの間は、一定時間たつと自動でゲームへ進む
/// </summary>
class Active : public ITitlePhase {
public:
	void Enter(TitleScene* scene) override;
	void Update(TitleScene* scene, float deltaTime) override;
	void Exit(TitleScene* scene) override;
	void DrawImGui(TitleScene* scene) override;

	bool IsDiving() const override { return false; }
	float GetDiveProgress() const override { return 1.0f; }

private:
	// デモモードでゲームへ進むまでの経過時間
	float demoTimer_ = 0.0f;
};

}
