#pragma once
#include "../GameObject.h"

class EnemyBullet : public Bonjin::GameObject {
public:
	EnemyBullet(const Vector3& position, const Vector3& velocity, Camera* camera, MapChipField* mapChipField);
	~EnemyBullet() = default;

	// 衝突コールバックのオーバーライド
	void OnCollision(Bonjin::Collider* other) override;

protected:
	// マップ壁衝突時の消滅処理
	void OnMapCollision(const CollisionMapInfo& collisionMapinfo) override;

private:
	std::unique_ptr<Object3D> bulletModel_;

	// 弾サイズ
	static inline const float kWidth = 0.5f;
	static inline const float kHeight = 0.5f;
};
