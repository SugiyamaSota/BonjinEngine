#pragma once
#include"../interface/IScene.h"

#include"../bonjin/BonjinEngine.h"

namespace BonjinEngine
{
    class TitleScene : public IScene
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

        // 
        WorldTransform worldTransform_{};
        Sprite* sprite_{ nullptr };

        //
        //Sprite* sprite_ = nullptr;

    private:
        // --- ゲーム固有の関数 ---

    };
}