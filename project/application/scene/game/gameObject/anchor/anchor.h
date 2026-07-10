#pragma once
#include "../GameObject.h"
#include "../../mapchip/MapChipField.h"

class Player;

class Anchor : public Bonjin::GameObject {
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	Anchor(const Vector3& position, const Vector3& velocity, MapChipField* mapChipField, Player* player, Camera* camera);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~Anchor() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	// 当たり判定メソッドを追加
	bool isCollisionMap();

	bool GetStandBy() const { return isStandBy; }
	void SetStandBy(bool flag) { isStandBy = flag; }

	float GetAngle() const { return worldTransform_.rotate.z; }

	bool IsInstantResolved() const { return isInstantResolved_; }
	void SetInstantResolved(bool flag) { isInstantResolved_ = flag; }

	// 衝突コールバックのオーバーライド
	void OnCollision(Bonjin::Collider* other) override;
	void OnCollision();

private:
	// 親プレイヤーへの参照
	Player* player_ = nullptr;
	std::unique_ptr<Object3D> anchorModel_;

	// 当たり判定サイズ
	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

	enum Corner {
		kRightBottom,
		kLeftBottom,
		kRightTop,
		kLeftTop,
		kNumCorner
	};

	// 角の座標を取得するヘルパーメソッド
	Vector3 CornerPosition(const Vector3& center, Corner corner);

	// 待機状態
	bool isStandBy = false;
	bool isInstantResolved_ = false;
};
