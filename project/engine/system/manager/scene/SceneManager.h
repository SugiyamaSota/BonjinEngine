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
        void Finalize();

    public:
        //
        void Initialize();
        // 💡 全シーンを登録
        void AddScene(SceneType type, std::unique_ptr<IScene> scene);

        // 💡 エンジンのループから呼ばれる更新関数
        void Update(float deltaTime);

        // 💡 エンジンのループから呼ばれる描画関数
        void Draw();

    private:

        // シングルトン関連の禁止
        SceneManager() = default;
        ~SceneManager() = default;

        // 💡 現在アクティブなシーン
        IScene* currentScene_ = nullptr;
        // 💡 登録されたすべてのシーンを保持するマップ
        std::map<SceneType, std::unique_ptr<IScene>> scenes_;

        // 💡 シーン切り替えロジック
        void ChangeScene(SceneType nextSceneType);

        std::unique_ptr<Camera> camera_ = nullptr;

    };

}