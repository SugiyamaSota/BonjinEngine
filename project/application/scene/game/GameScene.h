#pragma once

#include "../interface/BaseScene.h"
#include "battle/BattleController.h"

namespace Bonjin {

enum class GamePhase {
	kHighlight,
	kPlay,
	kGoal,
};

	class GameScene : public BaseScene<GamePhase>
	{
	public:
		// --- オーバーライド関数 --- 
		virtual ~GameScene() = default;

		void Initialize(Camera* camera) override;

		void Unload()override;

		void Update(float deltaTime) override;

		void Draw() override;

		SceneType GetNextScene() const override;

		void DrawSceneImGui() override;

		const char* GetScenename()const override {
			return "GameScene";
		}

		// ハイライト演出などのため、入場演出はカスタム（GameScene側で制御）
		TransitionType GetTransitionInType() const override { return TransitionType::kCustom; }
		bool HasCustomTransitionIn() const override { return true; }

	private:
		// --- ゲーム固有の変数 ---
		std::unique_ptr<BattleController> battleController_;

		// --- ハイライト演出用 ---
		enum class HighlightState {
			kFadeIn,
			kCameraPan,
			kFadeOut,
			kPlayerFocusIn
		};

		HighlightState highlightState_ = HighlightState::kFadeIn;
		size_t currentHighlightIndex_ = 0;
		float highlightTimer_ = 0.0f;
		static constexpr float kFadeDuration = 0.5f;
		static constexpr float kPlayerFocusDuration = 0.8f;

		void UpdateHighlightPhase(float deltaTime);
		void FinishHighlight();
		void UpdatePlayPhase(float deltaTime);
	};

}
