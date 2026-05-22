#include "Debris.h"
#include <random>

void Debris::Initialize(Camera* camera, const Vector3& position, const Vector3& velocity) {
    worldTransform_ = {
        {0.2f, 0.2f, 0.2f}, // 敵より小さくする
        {0, 0, 0},
        position,
    };
    model_ = new Model();
    model_->LoadModel("cube");
    camera_ = camera;
    velocity_ = velocity;
    rotationSpeed_ = (float)std::rand() / RAND_MAX * 0.1f + 0.01f; // ランダムな回転速度
}

void Debris::Update() {
    // 速度に重力を適用
    velocity_.y -= kGravity * (1.0f / 60.0f) * (1.0f / 60.0f);

    // 速度に基づいて座標を更新
    worldTransform_.translate.x += velocity_.x;
    worldTransform_.translate.y += velocity_.y;
    worldTransform_.translate.z += velocity_.z;

    // モデルを回転
    worldTransform_.rotate.x += rotationSpeed_;
    worldTransform_.rotate.y += rotationSpeed_;
    worldTransform_.rotate.z += rotationSpeed_;

    // 行列の変換
    model_->Update(worldTransform_, camera_);

    // 死亡タイマー
    timer_ += 1.0f / 60.0f;
    if (timer_ >= kDebrisLifeTime) {
        isDead_ = true;
    }
}

void Debris::Draw() {
    model_->Draw();
}