#include "Core.h"
#include "BonjinEngine.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "GameScene.h"

using namespace Bonjin;

Core::Core() {}
Core::~Core() {}

void Core::Initialize() {
    Bonjin::Initialize(); // エンジン本体の初期化

    // シーンマネージャーのセットアップ
    auto sceneManager = SceneManager::GetInstance();
    sceneManager->Initialize();
    sceneManager->AddScene(SceneType::kTitle, new TitleScene());
    sceneManager->AddScene(SceneType::kGame, new GameScene());

    // 天球
    skydomeModel_ = std::make_unique<Model>();
    skydomeModel_->LoadModel("debugSkydome");
    skydome_ = std::make_unique<Skydome>();
    skydome_->Initialize(skydomeModel_.get(),sceneManager->GetCamera());

}

void Core::Run() {
    while (true) {
        if (WinApp::GetInstance()->ProcessMessage()) {
            break;
        }

        NewFrame();

        // 更新処理
        float deltaTime = Time::GetInstance()->GetDeltaTime();
        SceneManager::GetInstance()->Update(deltaTime);

        skydome_->Update();

        PreDraw();

        // 描画処理
        SceneManager::GetInstance()->Draw();

        skydome_->Draw();

        EndFrame();
    }
}

void Core::Finalize() {
    SceneManager::DestroyInstance();
    Bonjin::Finalize();
}