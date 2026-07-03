#include "EnemyBullet.h"
#include "../../mapchip/MapChipField.h"
#include "../../logic/Collision.h"

EnemyBullet::EnemyBullet(const Vector3& position, const Vector3& velocity, Camera* camera, MapChipField* mapChipField) {
	camera_ = camera;
	velocity_ = velocity;
	mapChipField_ = mapChipField;

	worldTransform_ = {
		{0.3f, 0.3f, 0.3f},
		{0.0f, 0.0f, 0.0f},
		position
	};

	model_ = std::make_unique<Object3D>();
	model_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	model_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
}

void EnemyBullet::Update() {
	// 直進移動
	worldTransform_.translate = Add(worldTransform_.translate, velocity_);

	// 壁衝突判定
	if (mapChipField_) {
		IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translate);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		if (type == MapChipType::kBlock) {
			isDead_ = true; // 壁に当たったら消滅
		}
	}

	model_->Update(worldTransform_, camera_);
}

void EnemyBullet::Draw() {
	if (!isDead_) {
		model_->Draw();
	}
}

AABB EnemyBullet::GetAABB() const {
	AABB aabb;
	float size = 0.3f;
	aabb.min = { worldTransform_.translate.x - size / 2.0f, worldTransform_.translate.y - size / 2.0f, worldTransform_.translate.z - size / 2.0f };
	aabb.max = { worldTransform_.translate.x + size / 2.0f, worldTransform_.translate.y + size / 2.0f, worldTransform_.translate.z + size / 2.0f };
	return aabb;
}
