#pragma once
#include "../interface/BaseScene.h"
#include "../bonjin/BonjinEngine.h"
#include "TextSprite.h"
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

	// クリア / ゲームオーバー状態の設定
	static void SetIsClear(bool isClear) { isClear_ = isClear; }
	static bool IsClear() { return isClear_; }

private:
	static inline bool isClear_ = true;

	std::unique_ptr<TextSprite> gameClearText_ = nullptr;
	std::unique_ptr<TextSprite> gameOverText_ = nullptr;
	std::unique_ptr<TextSprite> spaceToTitleText_ = nullptr;
	float timer_ = 0.0f;
};

}
