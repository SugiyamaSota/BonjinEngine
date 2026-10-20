#pragma once

#include "../../engine/interface/BaseScene.h"
#include <string>

namespace Bonjin {

class TitleScene;

/// <summary>
/// TitleSceneのフェーズ基底インターフェース
/// </summary>
class ITitlePhase : public IScenePhase<TitleScene> {
public:
	virtual ~ITitlePhase() = default;

	// ダイブ演出中かどうか
	virtual bool IsDiving() const { return false; }

	// ダイブの進行度 (0.0f - 1.0f)
	virtual float GetDiveProgress() const { return 1.0f; }
};

/// <summary>
/// タイトル待機・操作フェーズ
/// </summary>
class TitlePhaseActive : public ITitlePhase {
public:
	void Enter(TitleScene* scene) override;
	void Update(TitleScene* scene, float deltaTime) override;
	void Exit(TitleScene* scene) override;

	bool IsDiving() const override { return false; }
	float GetDiveProgress() const override { return 1.0f; }
};

/// <summary>
/// ズーム＆激震シェイク突入ダイブフェーズ
/// </summary>
class TitlePhaseDive : public ITitlePhase {
public:
	explicit TitlePhaseDive(const std::string& targetSceneType);

	void Enter(TitleScene* scene) override;
	void Update(TitleScene* scene, float deltaTime) override;
	void Exit(TitleScene* scene) override;

	bool IsDiving() const override { return true; }
	float GetDiveProgress() const override;

private:
	std::string targetSceneType_;
	float timer_ = 0.0f;
	static constexpr float kDiveDuration = 1.0f;
};

}
