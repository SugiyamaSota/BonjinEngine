#pragma once
#include"BonjinEngine.h"
#include <map>
#include<memory>

#include"../interface/IScene.h"

namespace Bonjin
{

    class SceneManager {
    public:
        // シングルトン化 (エンジンの中核なので)
        static SceneManager* GetInstance();
        static void DestroyInstance();

    public:
        //
        void Initialize();
        // 💡 全シーンを登録
        void AddScene(SceneType type, IScene* scene);

        // 💡 エンジンのループから呼ばれる更新関数
        void Update(float deltaTime);

        // 💡 エンジンのループから呼ばれる描画関数
        void Draw();

        Camera* GetCamera()const { return camera; };

        float GetFadeIOAlpha()const { return currentScene_->GetFadeIOAlpha(); }

    private:
        static SceneManager* instance;

        // シングルトン関連の禁止
        SceneManager() = default;
        ~SceneManager() = default;
        // ... (コピー禁止など)

        // 💡 現在アクティブなシーン
        IScene* currentScene_ = nullptr;
        // 💡 登録されたすべてのシーンを保持するマップ
        std::map<SceneType, IScene*> scenes_;

        // 💡 シーン切り替えロジック
        void ChangeScene(SceneType nextSceneType);

        Camera* camera = nullptr;

    };

}