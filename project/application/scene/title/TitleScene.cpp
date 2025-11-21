#include "TitleScene.h"

using namespace BonjinEngine;

void TitleScene::Initialize(Camera* camera) {
    currentSceneType_ = SceneType::kTitle;
    nextSceneType_ = SceneType::kTitle;

    this->camera_ = camera;

    WorldTransform wt = InitializeWorldTransform();
    model_ = new Model;
    model_->LoadModel("plane");
    model_->Update(wt, camera);

  /*  sprite_ = new Sprite;
    sprite_->Initialize(Vector3{ 0.f,0.f,0.f }, Color::White, Vector3{ 0.5f,0.5f,0.f }, Vector2{ 256,256 }, "uvChecker.png");*/
}

void TitleScene::Unload() {
    if (model_ != nullptr) {
        delete model_;
        model_ = nullptr;
    }

   /* if (sprite_ != nullptr) {
        delete sprite_;
        sprite_ = nullptr;
    }*/
}

void TitleScene::Update(float deltaTime) {
    WorldTransform wt = InitializeWorldTransform();
    model_->Update(wt, camera_);
   // sprite_->Update(Vector3{ 640.f,360.f,0.f }, Color::Blue);

    if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
        nextSceneType_ = SceneType::kGame;
    }
}

void TitleScene::Draw() {
    model_->Draw();

    //sprite_->Draw();
}

void TitleScene::DrawSceneImGui() {

}

SceneType TitleScene::GetNextScene() const {
    return nextSceneType_;
}