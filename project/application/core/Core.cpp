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

    fadeIOSprite_ = std::make_unique<Sprite>();
    fadeIOSprite_->Initialize("uvChecker.png");
    fadeIOSprite_->Scale() = { 1.f,1.f };
    fadeIOSprite_->Rotate() = { 0.f,0.f };
    fadeIOSprite_->Translate() = { 0.f, 0.f};
    fadeIOSprite_->Anchor() = { 0.f,0.f };
    fadeIOSprite_->Color() = { 0.f,0.f,0.f,1.f };
    fadeIOSprite_->Size() = { 1280.f, 720.f };

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
        

        fadeIOSprite_->Color() = { 0.f,0.f,0.f,SceneManager::GetInstance()->GetFadeIOAlpha() };

        skydome_->Update();
        fadeIOSprite_->Update();

        PreDraw();

        skydome_->Draw();
        // 描画処理
        SceneManager::GetInstance()->Draw();
        
        fadeIOSprite_->Draw();

        EndFrame();
    }
}

void Core::Finalize() {
    SceneManager::DestroyInstance();
    Bonjin::Finalize();
}