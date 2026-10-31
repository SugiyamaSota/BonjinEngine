#pragma once
#include "../interface/BaseScene.h"
#include "../bonjin/BonjinEngine.h"
#include "TextSprite.h"
#include <memory>

namespace Bonjin {

class ResultScene : public BaseScene<ResultScene> {
public:
	virtual ~ResultScene() = default;

	void Initialize(Camera* camera) override;
	void Unload() override;
	void Update(float deltaTime) override;
	void Draw() override;
	SceneType GetNextScene() const override;
	const char* GetScenename() const override { return "ResultScene"; }

	// フェードイン・フェードアウトはシンプルな暗転（SimpleFade）
	TransitionType GetTransitionInType() const override { return TransitionType::kFadeBlack; }
	float GetTransitionInDuration() const override { return 0.6f; }
	TransitionType GetTransitionOutType() const override { return TransitionType::kFadeBlack; }
	float GetTransitionOutDuration() const override { return 0.6f; }

	// クリア / ゲームオーバー状態の設定
	static void SetIsClear(bool isClear) { isClear_ = isClear; }
	static bool IsClear() { return isClear_; }

private:
	static inline bool isClear_ = true;

	std::unique_ptr<Sprite> background_ = nullptr; // 全画面の黒背景
	std::unique_ptr<TextSprite> gameClearText_ = nullptr;
	std::unique_ptr<TextSprite> gameOverText_ = nullptr;
	std::unique_ptr<TextSprite> spaceToTitleText_ = nullptr;
	float timer_ = 0.0f;
};

}
