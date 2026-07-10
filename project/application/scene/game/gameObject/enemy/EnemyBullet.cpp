#include "EnemyBullet.h"
#include "../../mapchip/MapChipField.h"
#include "../../logic/Collision.h"
#include "../player/Player.h"

using namespace Bonjin;

EnemyBullet::EnemyBullet(const Vector3& position, const Vector3& velocity, Camera* camera, MapChipField* mapChipField) {
	mapChipField_ = mapChipField;
	width_ = kWidth;
	height_ = kHeight;

	bulletModel_ = std::make_unique<Object3D>();
	bulletModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	bulletModel_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

	GameObjectInitConfig config;
	config.physicsType = PhysicsType::Linear;
	config.hasCollider = true;
	config.categoryAttr = kAttributeEnemyBullet;
	config.collisionMask = kAttributePlayer;
	config.tag = "EnemyBullet";

	GameObject::Initialize(bulletModel_.get(), camera, position, config);

	velocity_ = velocity;

	worldTransform_.scale = { 0.3f, 0.3f, 0.3f };
}

void EnemyBullet::OnCollision(Bonjin::Collider* other) {
	if (other->GetCategoryAttr() == kAttributePlayer) {
		Player* player = static_cast<Player*>(other->GetOwner());
		if (!player->GetIsInvincible()) {
			player->ApplyDamage(1);
		}
		SetDead(true);
	}
}

void EnemyBullet::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	if (collisionMapinfo.isHitWall_ || collisionMapinfo.isHotTop_ || collisionMapinfo.isLandin_) {
		SetDead(true);
	}
}
