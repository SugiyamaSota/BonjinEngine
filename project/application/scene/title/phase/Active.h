#pragma once

#include "ITitlePhase.h"

namespace Bonjin::TitlePhase {

/// <summary>
/// タイトル待機・操作フェーズ
/// </summary>
class Active : public ITitlePhase {
public:
	void Enter(TitleScene* scene) override;
	void Update(TitleScene* scene, float deltaTime) override;
	void Exit(TitleScene* scene) override;

	bool IsDiving() const override { return false; }
	float GetDiveProgress() const override { return 1.0f; }
};

}
