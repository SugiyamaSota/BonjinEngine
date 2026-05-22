#include "Enemy.h"
#include <cmath>
#include <numbers>
#include <random>
#include <list>
#include <algorithm>

using namespace Bonjin;

void Enemy::Initialize(Object3D* model, Camera* camera, const Vector3& position) {

	worldTransform_ = {
		{1,1,1},
		{0,0,0},
		position,
	};

	model_ = model;

	camera_ = camera;

	velocity_ = { -kWalkSpeed, 0, 0 };

	walkTimer_ = 0.0f;

	
	lockedOnSprite_ = new Sprite();
	lockedOnSprite_->Initialize("resources/textures/default.png");

	Vector3 worldPos = GetWorldPosition();
	Vector3 screenPos = camera_->Project(worldPos);
	lockedOnSprite_->Translate() = { screenPos.x,screenPos.y };
}

void Enemy::Update() {

	// 移動処理
	//worldTransform_.translate = Add(worldTransform_.translate, velocity_);

	// 旋回制御
	TurningControl();

	// 縦方向の動き
	walkTimer_ += 1.0f / 60.0f;

	float param = std::sin(kPi * 2 * walkTimer_ / kWalkTimer);
	float radian = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;
	worldTransform_.rotate.z = radian * (kPi / 180.0f);

	if (walkTimer_ >= kWalkTimer) {
		walkTimer_ = 0.0f;
	}

	// 行列の変換
	model_->Update(worldTransform_, camera_);

    lockedOnSprite_->Update();
}


void Enemy::Draw() { 
	model_->Draw(); 
	
	if (isLockedOn_) {
		lockedOnSprite_->Draw();
	}
}

void Enemy::TurningControl() {
	{
		float destinationRotationYTable[] = {
			std::numbers::pi_v<float> *-4.0f / 2.0f,
			std::numbers::pi_v<float> *2.0f / 2.0f,
		};
		// 状況に応じた角度を取得する
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		// 自キャラの角度を設定する
		worldTransform_.rotate.y = destinationRotationY;
	}
}

Vector3 Enemy::GetWorldPosition() {
	return worldTransform_.translate;
}


AABB Enemy::GetAABB() {
	AABB aabb;
	Vector3 worldPos = GetWorldPosition();
	aabb.min = { worldPos.x - kWidth_ / 2.0f, worldPos.y - kHeight_ / 2.0f, worldPos.z - kWidth_ / 2.0f };
	aabb.max = { worldPos.x + kWidth_ / 2.0f, worldPos.y + kHeight_ / 2.0f, worldPos.z + kWidth_ / 2.0f };
	return aabb;
}

void Enemy::OnCollision() {
	isLockedOn_ = true;
}

void Enemy::SetIsLockedOn(bool frag) {
	isLockedOn_ = frag;
	if (frag == true) {
		model_->SetColor(Vector4{ 1, 0, 0, 1 });
	} else {
		model_->SetColor(Vector4{ 1,1,1,1 });
	}
}

std::list<std::unique_ptr<Debris>> Enemy::ExplodeAndGetDebris() {
	std::list<std::unique_ptr<Debris>> debrisList;
	const int kDebrisCount = 20; // 生成する破片の数
	std::random_device seed_gen;
	std::mt19937 engine(seed_gen());
	std::uniform_real_distribution<> dist(-1.0f, 1.0f);

	for (int i = 0; i < kDebrisCount; ++i) {
		// 破片の初期速度をランダムに決定
		Vector3 randomVelocity = {
			(float)dist(engine) * 0.1f,
			(float)dist(engine) * 0.1f + 0.15f, // 上方向の速度を少し強めにする
			(float)dist(engine) * 0.1f
		};

		// 新しいDebrisオブジェクトを生成
		std::unique_ptr<Debris> newDebris = std::make_unique<Debris>();
		// Initializeにmodel_を渡す
		newDebris->Initialize(camera_, worldTransform_.translate, randomVelocity);
		debrisList.push_back(std::move(newDebris));
	}
	return debrisList;
}