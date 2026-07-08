#pragma once
#include "../interface/BaseScene.h"
#include "../bonjin/BonjinEngine.h"
#include "Sprite.h"
#include <memory>

namespace Bonjin {

enum class ResultPhase {
	kFadeIn,
	kActive,
	kFadeOut,
};

class ResultScene : public BaseScene<ResultPhase> {
public:
	virtual ~ResultScene() = default;

	void Initialize(Camera* camera) override;
	void Unload() override;
	void Update(float deltaTime) override;
	void Draw() override;
	SceneType GetNextScene() const override;
	const char* GetScenename() const override { return "ResultScene"; }

private:
	std::unique_ptr<Sprite> clearSprite_;
};

}
