#pragma once

#include "../interface/BaseScene.h"
#include "battle/BattleController.h"

namespace Bonjin {

class GameScene : public BaseScene<GameScene>
	{
	public:
		// --- オーバーライド関数 --- 
		virtual ~GameScene() = default;

		void Initialize(Camera* camera) override;

		void Unload() override;

		void Update(float deltaTime) override;

		void Draw() override;

		SceneType GetNextScene() const override;

		void DrawSceneImGui() override;

		const char* GetScenename() const override {
			return "GameScene";
		}

		// ハイライト演出などのため、入場演出はカスタム（GameScene側で制御）
		TransitionType GetTransitionInType() const override { return TransitionType::kCustom; }
		bool HasCustomTransitionIn() const override { return true; }

		// フェーズから参照するアクセサ
		BattleController* GetBattleController() const { return battleController_.get(); }

	private:
		// --- ゲーム固有の変数 ---
		std::unique_ptr<BattleController> battleController_;
	};

}
