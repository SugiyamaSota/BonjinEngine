#pragma once

#include "ITitlePhase.h"
#include <string>

namespace Bonjin::TitlePhase {

/// <summary>
/// ズーム＆激震シェイク突入ダイブフェーズ
/// </summary>
class Dive : public ITitlePhase {
public:
	explicit Dive(const std::string& targetSceneType);

	void Enter(TitleScene* scene) override;
	void Update(TitleScene* scene, float deltaTime) override;
	void Exit(TitleScene* scene) override;

	bool IsDiving() const override { return true; }
	float GetDiveProgress() const override;

private:
	std::string targetSceneType_;
	float timer_ = 0.0f;
	bool transitionRequested_ = false;
	static constexpr float kDiveDuration = 1.0f;
};

}
