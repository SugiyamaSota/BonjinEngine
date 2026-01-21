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
		//float phaseTimer_ = 0.0f;

		// タイトルモデル(Anchor)
		std::unique_ptr <Model> titleModel_ = nullptr;
		WorldTransform titleWT_;

		// スタートするHUDのモデル
		std::unique_ptr <Model> startHUDModel_ = nullptr;
		WorldTransform startHUDWT_;

		// HUDモデル回転用変数
		float hudRotationY_ = 0.0f;      // 現在の角度
		float hudRotationSpeed_ = 0.0f;

	private:
		// --- ゲーム固有の関数 ---

	};
}