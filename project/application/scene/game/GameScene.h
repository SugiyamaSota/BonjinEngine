#pragma once
#include "../interface/IScene.h" // ISceneをインクルード

#include"../bonjin/BonjinEngine.h"

#include"Sprite.h"

namespace Bonjin
{
	class GameScene : public IScene
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

	private:
		// --- ゲーム固有の変数 ---
		// スプライト
		Bonjin::Sprite sprite_;

	private:
		// --- ゲーム固有の関数 ---
	};
}