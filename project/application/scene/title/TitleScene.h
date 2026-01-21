#pragma once
#include"../interface/BaseScene.h"

#include"../bonjin/BonjinEngine.h"

namespace Bonjin
{

	enum class TitlePhase {
		kFadeIn,
		kActive,
		kFadeOut,
	};

	class TitleScene : public BaseScene<TitlePhase>
	{
	public:
		// --- オーバーライド関数 ---
		virtual ~TitleScene() = default;

		void Initialize(Camera* camera) override;

		void Unload()override;

		void Update(float deltaTime) override;

		void Draw() override;

		void DrawSceneImGui()override;

		SceneType GetNextScene() const override;

		const char* GetScenename()const override
		{
			return "TitleScene";
		}

	private:
		// --- ゲーム固有の変数 ---
		float phaseTimer_ = 0.0f;

		// タイトルモデル(Anchor)
		std::unique_ptr <Model> titleModel_ = nullptr;
		WorldTransform titleWT_;

		// スタートするHUDのモデル
		std::unique_ptr <Model> startHUDModel_ = nullptr;
		WorldTransform startHUDWT_;

	private:
		// --- ゲーム固有の関数 ---

	};
}