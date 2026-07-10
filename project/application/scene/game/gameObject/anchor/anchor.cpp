#include "Anchor.h"
#include "../player/Player.h"
#include "../enemy/Enemy.h"
#include <array>
#include <numbers>

using namespace Bonjin;

Anchor::Anchor(const Vector3& position, const Vector3& velocity, MapChipField* mapChipField, Player* player, Camera* camera)
	: player_(player) {
	mapChipField_ = mapChipField;
	width_ = kWidth;
	height_ = kHeight;

	anchorModel_ = std::make_unique<Object3D>();
	anchorModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	anchorModel_->SetColor({ 0, 0, 1, 1 });
	anchorModel_->SetEnableEnableEnvironmentMap(false);

	GameObjectInitConfig config;
	config.physicsType = PhysicsType::Linear;
	config.hasCollider = true;
	config.categoryAttr = kAttributeAnchor;
	config.collisionMask = kAttributeEnemy;
	config.tag = "Anchor";

	GameObject::Initialize(anchorModel_.get(), camera, position, config);

	velocity_ = velocity;

	if (velocity_.x != 0.0f || velocity_.y != 0.0f) {
		float angle = atan2f(velocity_.y, velocity_.x) + std::numbers::pi_v<float> / 2.0f;
		worldTransform_.rotate.z = angle;
	}
	isStandBy = false;
	isInstantResolved_ = false;
}

void Anchor::Update() {
	// マップ衝突判定
	isCollisionMap();

	if (isStandBy) {
		if (model_) {
			model_->Update(worldTransform_, camera_);
		}
		if (collider_) {
			collider_->SetAABB(GetAABB());
		}
		return;
	}

	GameObject::Update();
}

void Anchor::OnCollision(Bonjin::Collider* other) {
	if (other->GetCategoryAttr() == kAttributeEnemy) {
		Enemy* enemy = static_cast<Enemy*>(other->GetOwner());
		enemy->OnCollision();
		if (player_) {
			player_->AddLockedOnEnemy(enemy);
		}
		isStandBy = true;
		SetDead(true);

		if (player_) {
			player_->EmitAnchorHitEffect(GetPosition());
		}
	}
}

void Anchor::OnCollision() {
	isStandBy = true;
	SetDead(true);
}

bool Anchor::isCollisionMap() {
	if (mapChipField_ == nullptr) {
		return false;
	}

	Vector3 nextPosition = Add(worldTransform_.translate, velocity_);

	std::array<Vector3, kNumCorner> corners;
	for (uint32_t i = 0; i < kNumCorner; ++i) {
		corners[i] = CornerPosition(nextPosition, static_cast<Corner>(i));
	}

	for (const auto& corner : corners) {
		IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(corner);
		MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		if (mapChipType == MapChipType::kBlock) {
			isStandBy = true;
			return true;
		}
	}

	return false;
}

Vector3 Anchor::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
		{+kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
		{-kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
		{+kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
		{-kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
	};
	return Add(center, offsetTable[static_cast<uint32_t>(corner)]);
}
