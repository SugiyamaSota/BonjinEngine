#pragma once
#include "../../logic/Data.h"
#include "Object3D.h"
#include <memory>

class MapChipField;
class Camera;

class EnemyBullet {
private:
	WorldTransform worldTransform_{};
	Vector3 velocity_ = {};
	Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	std::unique_ptr<Object3D> model_;

	bool isDead_ = false;

public:
	EnemyBullet(const Vector3& position, const Vector3& velocity, Camera* camera, MapChipField* mapChipField);
	~EnemyBullet() = default;

	void Update();
	void Draw();

	bool IsDead() const { return isDead_; }
	void SetDead(bool flag) { isDead_ = flag; }
	Vector3 GetPosition() const { return worldTransform_.translate; }
	AABB GetAABB() const;
};
