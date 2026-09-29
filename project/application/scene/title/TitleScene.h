#pragma once
#include "../interface/BaseScene.h"
#include "../bonjin/BonjinEngine.h"
#include "TextSprite.h"
#include <memory>

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

        void Unload() override;

        void Update(float deltaTime) override;

        void Draw() override;

        SceneType GetNextScene() const override;

        const char* GetScenename() const override
        {
            return "TitleScene";
        }

    private:
        // --- ゲーム固有の変数 ---
        std::unique_ptr<TextSprite> titleText_ = nullptr;
        std::unique_ptr<TextSprite> startText_ = nullptr;
        float timer_ = 0.0f;

    private:
        // --- ゲーム固有の関数 ---

    };
}