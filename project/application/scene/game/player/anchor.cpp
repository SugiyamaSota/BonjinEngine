#include "Anchor.h"
#include <array>
#include<numbers>

// コンストラクタを修正
Anchor::Anchor(const Vector3& position, const Vector3& velocity, MapChipField* mapChipField)
	: position_(position), velocity_(velocity), mapChipField_(mapChipField) {
	model_ = new Model();
	model_->LoadModel("anchor");
	model_->SetColor({ 1, 1, 1, 1 });
	worldTransform_ = InitializeWorldTransform();
	worldTransform_.scale = { 1.0f,1.0f,1.0f };
	if (velocity_.x != 0.0f || velocity_.y != 0.0f) {
		float angle = atan2f(velocity_.y, velocity_.x) + std::numbers::pi_v<float> / 2.0f;

		// worldTransform_のZ軸回転に適用
		worldTransform_.rotate.z = angle;
	}
	worldTransform_.translate = position;
	isStandBy = false;
}

Anchor::~Anchor() {
	delete model_;
}

void Anchor::Update(Camera& camera) {
	isCollisionMap();
	if (isStandBy==true) {
		model_->Update(worldTransform_, &camera);
		return;
	}

	// 速度を反映させて位置を更新
	worldTransform_.translate = Add(worldTransform_.translate, velocity_);

	model_->Update(worldTransform_, &camera);
}

void Anchor::Draw() const {
	model_->Draw();
}

// プレイヤーと同様の当たり判定ロジックを実装
bool Anchor::isCollisionMap() {
	if (mapChipField_ == nullptr) {
		return false;
	}

	// 次のフレームの移動後の座標を計算
	Vector3 nextPosition = Add(worldTransform_.translate, velocity_);

	// 4つの角の座標の取得
	std::array<Vector3, kNumCorner> corners;
	for (uint32_t i = 0; i < kNumCorner; ++i) {
		corners[i] = CornerPosition(nextPosition, static_cast<Corner>(i));
	}

	// 各角がブロックと衝突しているかチェック
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

// プレイヤーと同様の角の座標取得メソッド
Vector3 Anchor::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
		{+kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
		{-kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
		{+kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
		{-kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
	};
	return Add(center, offsetTable[static_cast<uint32_t>(corner)]);
}

Vector3 Anchor::GetWorldPosition() {
	return worldTransform_.translate;
}

AABB Anchor::GetAABB() {
	AABB aabb;
	Vector3 worldPos = GetWorldPosition();
	aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
	aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };
	return aabb;
}

void Anchor::OnCollision() {
	isStandBy = true;
	isDead_ = true;
}