#define NOMINMAX
#include "Player.h"
#include"../mapchip/MapChipField.h"
#include <algorithm>
#include <numbers>
#include<array>
#include"../enemy/Enemy.h"

void Player::Initialize(Model* model, Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;

	worldTransform_ = {
		{1,1,1},
		{0,0,0},
		position,
	};

	camera_ = camera;

	onGround_ = false;

}

void Player::Move() {
	// 移動入力
	// 接地状態
	if (onGround_) {
		// ゲームパッドの左スティックのX軸の値を取得
		long lStickX = Input::GetInstance()->GetPadLStickX();

		// 💡 キーボードとスティックの入力を統合して左右の移動フラグを作成
		// 💡 キーボードの移動には、押し続けている間反応するIsPress()を使用します
		bool isMovingRight = (lStickX > kPadDeadZone_) || Input::GetInstance()->IsPress(DIK_D);
		bool isMovingLeft = (lStickX < -kPadDeadZone_) || Input::GetInstance()->IsPress(DIK_A);

		Vector3 acceleration = {};

		// 左右移動操作
		if (isMovingRight) { // 右に移動
			if (velocity_.x < 0.0f) {
				// 向きが変わるときは減速させる
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x += kAcceleration;
			// 向きが変わったら旋回処理を開始
			if (lrDirection_ != LRDirection::kRight) {
				lrDirection_ = LRDirection::kRight;
				turnFirstRotationY_ = worldTransform_.rotate.y;
				turnTimer_ = kTimeTurn;
			}
		} else if (isMovingLeft) { // 左に移動
			if (velocity_.x > 0.0f) {
				// 向きが変わるときは減速させる
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x -= kAcceleration;
			// 向きが変わったら旋回処理を開始
			if (lrDirection_ != LRDirection::kLeft) {
				lrDirection_ = LRDirection::kLeft;
				turnFirstRotationY_ = worldTransform_.rotate.y;
				turnTimer_ = kTimeTurn;
			}
		}

		// 加速/減速
		velocity_ = Add(velocity_, acceleration);

		// 最大速度制限
		velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);

		// 💡 入力がない場合は減衰をかける
		if (!isMovingRight && !isMovingLeft) {
			// 非入力時は移動減衰をかける
			velocity_.x *= (1.0f - kAttenuation);
		}

		// Aボタンでジャンプ (DIK_SPACEはIsTriggerのままでOK)
		if (Input::GetInstance()->IsPadPress(0) || Input::GetInstance()->IsTrigger(DIK_SPACE)) {
			velocity_ = Add(velocity_, Vector3(0, kJumpAcceleration, 0));
		}
	} else {
		// 空中
		// === ここから空中での移動処理を修正 ===
		long lStickX = Input::GetInstance()->GetPadLStickX();

		// 💡 空中でのキーボード入力とスティック入力を統合
		bool isMovingRightInAir = (lStickX > kPadDeadZone_) || Input::GetInstance()->IsPress(DIK_D);
		bool isMovingLeftInAir = (lStickX < -kPadDeadZone_) || Input::GetInstance()->IsPress(DIK_A);

		// 左右移動操作 (空中での左右入力)
		if (isMovingRightInAir || isMovingLeftInAir) {
			Vector3 acceleration = {};
			if (isMovingRightInAir) { // 右に移動
				acceleration.x += kAccelerationInAir;
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotate.y;
					turnTimer_ = kTimeTurn;
				}
			} else if (isMovingLeftInAir) { // 左に移動
				acceleration.x -= kAccelerationInAir;
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotate.y;
					turnTimer_ = kTimeTurn;
				}
			}
			velocity_ = Add(velocity_, acceleration);

			// 最大速度制限
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
		}
		// === ここまで空中での移動処理を修正 ===

		velocity_ = Add(velocity_, Vector3(0, -kGravityAcceleration, 0));
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

// 移動量を加味した衝突判定
void Player::isCollisionMap(CollisionMapInfo& info) {
	isCollisionMapLeft(info);
	isCollisionMapRight(info);
	isCollisionMapTop(info);
	isCollisionMapBottom(info);
}

// 天井の衝突判定
void Player::isCollisionMapTop(CollisionMapInfo& info) {
	std::array<Vector3, 4> positionsNew;

	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translate, info.movement_), static_cast<Corner>(i));
	}

	if (info.movement_.y <= 0) {
		return;
	}

	MapChipType mapChipType;
	// 真上の当たり判定
	bool hit = false;
	// 左上
	IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		hit = true;
	}
	// 左上
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		// めり込みを排除する方向に移動量を設定
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
		info.movement_.y = std::max(0.0f, velocity_.y);
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
		info.movement_.y = std::max(0.0f, velocity_.y);
		// 天井に当たったことを記録
		info.isHotTop_ = true;
	}
}

void Player::isCollisionMapBottom(CollisionMapInfo& info) {
	std::array<Vector3, 4> positionsNew;

	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translate, info.movement_), static_cast<Corner>(i));
	}

	if (info.movement_.y >= 0) {
		return;
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	// 真下の当たり判定
	bool hit = false;
	// 左下
	IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}
	// 右下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		// めり込みを排除する方向に移動量を設定
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
		Vector3 temporaryPosition = worldTransform_.translate;
		temporaryPosition.y -= kHeight / 2;
		IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(temporaryPosition);
		if (indexSetNow.yIndex != indexSet.yIndex) {
			Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			// 修正: プレイヤーの底面が地面の上端に乗るように
			info.movement_.y = std::max(0.0f, rect.top - (worldTransform_.translate.y - kHeight / 2));
			info.isLandin_ = true;
		}
	}
}

// 右の壁の衝突判定
void Player::isCollisionMapRight(CollisionMapInfo& info) {
	// 右移動しているか？
	if (info.movement_.x <= 0) {
		return;
	}

	// 四つの角の座標の取得
	std::array<Vector3, 4> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translate, info.movement_), static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	// 真下の当たり判定
	bool hit = false;
	// 右上
	IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		hit = true;
	}
	// 右下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		hit = true;
	}

	// ブロックに当たっているか？
	if (hit) {
		// めり込みを排除する方向に移動量を設定
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
		Vector3 temporaryPosition = worldTransform_.translate;
		temporaryPosition.x += kWidth / 2;
		IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(temporaryPosition);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			// 修正: プレイヤーの底面が地面の上端に乗るように
			info.movement_.x = std::max(0.0f, rect.right - worldTransform_.translate.x - kWidth);
			info.isHitWall_ = true;
		}
	}
}

// 左の壁の衝突判定
void Player::isCollisionMapLeft(CollisionMapInfo& info) {
	// 左移動しているか？
	if (info.movement_.x >= 0) {
		return;
	}

	// 四つの角の座標の取得
	std::array<Vector3, 4> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translate, info.movement_), static_cast<Corner>(i));
	}

	MapChipType mapChipType;

	// 真下の当たり判定
	bool hit = false;
	// 左上
	IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		hit = true;
	}

	// 左下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		hit = true;
	}

	// ブロックに当たっているか？
	if (hit) {
		// めり込みを排除する方向に移動量を設定
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
		Vector3 temporaryPosition = worldTransform_.translate;
		temporaryPosition.x -= kWidth / 2;
		IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(temporaryPosition);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			// 修正: プレイヤーの底面が地面の上端に乗るように
			info.movement_.x = std::max(0.0f, rect.right - worldTransform_.translate.x + kWidth / 2);
			info.isHitWall_ = true;
		}
	}
}

void Player::CheckLanding(const CollisionMapInfo& info) {
	if (onGround_) {
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {

			std::array<Vector3, 4> positionsNew;

			for (uint32_t i = 0; i < positionsNew.size(); ++i) {
				positionsNew[i] = CornerPosition(Add(worldTransform_.translate, info.movement_), static_cast<Corner>(i));
			}

			MapChipType mapChipType;
			// 真下の当たり判定
			bool hit = false;
			// 左下
			IndexSet indexSet;
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(Add(positionsNew[kLeftBottom], Vector3(0, -0.01f, 0)));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}
			// 右下
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(Add(positionsNew[kRightBottom], Vector3(0, -0.01f, 0)));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}
			if (!hit) {
				onGround_ = false;
			}
		}
	} else {
		if (info.isLandin_) {
			onGround_ = true;
			velocity_.y *= (1.0f - 0.5f);
			velocity_.y = 0.0f; 
		}
	}
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
		{+kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
		{-kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
		{+kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
		{-kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
	};
	return Add(center, offsetTable[static_cast<uint32_t>(corner)]);
}

//判定結果を反映して移動
void Player::ResolveCollision(const CollisionMapInfo& info) {
	worldTransform_.translate = Add(info.movement_, worldTransform_.translate);
}

// 天井に当たっている場合
void Player::TopCollisionReaction(const CollisionMapInfo& info) {
	if (info.isHotTop_) {
		velocity_.y *= (1.0f - kAttenuationTop);
	}
}

void Player::WallCollisionReaction(const CollisionMapInfo& info) {
	if (info.isHitWall_) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

void Player::Update() {
	HandleLockOnRemovalInput();


	// 1.移動処理
	Move();

	ImGui::Begin("player");
	if (onGround_ == true) {
		ImGui::Text("onGround : true");
	} else {
		ImGui::Text("onGround : false");
	}
	ImGui::End();

	if (isKnockedBack_) {
		knockbackTimer_ -= 1.0f / 60.0f; // 1秒間に60フレームを想定
		if (knockbackTimer_ <= 0.0f) {
			isKnockedBack_ = false;
			knockbackTimer_ = 0.0f;
		}
	}

	// 2.移動量を加味した衝突判定
	CollisionMapInfo collisionMapinfo;      // 衝突情報初期化
	collisionMapinfo.movement_ = velocity_; // 移動量に速度をコピー
	isCollisionMap(collisionMapinfo);       // マップ衝突チェック

	// 3.判定結果を反映して移動
	ResolveCollision(collisionMapinfo);

	// 4.天井に当たっている場合
	TopCollisionReaction(collisionMapinfo);
	WallCollisionReaction(collisionMapinfo);

	// 着地フラグ
	CheckLanding(collisionMapinfo);

	// 旋回制御
	if (turnTimer_ > 0.0f) {

		{
			turnTimer_ -= 1.0f / 60.0f;
			// 左右の自キャラ角度テーブル
			float destinationRotationYTable[] = {
				std::numbers::pi_v<float> *-4.0f / 2.0f,
				std::numbers::pi_v<float> *2.0f / 2.0f,
			};
			// 状況に応じた角度を取得する
			float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
			// 自キャラの角度を設定する
			worldTransform_.rotate.y = destinationRotationY * (1.0f - turnTimer_);
		}
	}

	model_->Update(worldTransform_,camera_);

	// XボタンでshootAnchor
	if (Input::GetInstance()->IsPadTrigger(2) || Input::GetInstance()->IsTrigger(DIK_J)) {
		shootAnchor();
	}

	// アンカーの更新
	CollisionMapInfo info; // マップの衝突情報

	if (anchor_ != nullptr) {
		// アンカーを更新
		anchor_->Update(*camera_);
		// 衝突フラグが立っているかチェック
		if (anchor_->IsDead()) {
			anchor_ = nullptr; // アンカーを削除
		}
	}

	// Yボタンでテレポート
	if (Input::GetInstance()->IsPadTrigger(1) || Input::GetInstance()->IsTrigger(DIK_K)) {
		// アンカーが存在し、isStandByがtrueの場合
		if (anchor_ && anchor_->GetStandBy()) {
			// アンカーの位置を取得
			Vector3 anchorPos = anchor_->GetPosition();

			// テレポート先の座標を決定
			Vector3 teleportPosition = anchorPos;

			// === ここから修正 ===
			// キャラクターの高さの半分だけ上方向に移動
			teleportPosition.y = anchorPos.y + kHeight / 2.0f;
			// === ここまで修正 ===

			Vector3 anchorVelocity = anchor_->GetVelocity();

			// プレイヤーの移動方向を判定
			if (anchorVelocity.x > 0.0f) {
				// 壁にめり込まないように、アンカーの位置からプレイヤーの幅の半分だけ右にずらす
				teleportPosition.x -= kWidth / 2.0f;

			} else {
				// 壁にめり込まないように、アンカーの位置からプレイヤーの幅の半分だけ左にずらす
				teleportPosition.x += kWidth / 2.0f;

			}
			// プレイヤーを計算された座標にテレポート
			worldTransform_.translate = teleportPosition;

			// アンカーを消去
			anchor_ = nullptr;
		}
	}
}

void Player::Draw() {
	// 自キャラの描画処理
	model_->Draw();
	if (anchor_ != nullptr) {
		anchor_->Draw();
	}
}

void Player::shootAnchor() {
	// アンカーがすでに存在する場合は何もしない
	if (anchor_ != nullptr) {
		return;
	}

	// ゲームパッドの左スティックのX軸とY軸の値を取得
	long lStickX = Input::GetInstance()->GetPadLStickX();
	long lStickY = Input::GetInstance()->GetPadLStickY();

	// プレイヤーが向いている方向を考慮してアンカーを生成
	Vector3 initialVelocity;

	// 左右の向きに応じてX軸の初速を決定
	float xVelocity = (lrDirection_ == LRDirection::kRight) ? kAnchorSpeed : -kAnchorSpeed;

	// 上下の傾き具合で初速のY軸成分を決定
	if (lStickY > kAnchorDeadZone||Input::GetInstance()->IsPress(DIK_S)) {
		// 上に傾いている場合は45度上向き
		xVelocity = (lrDirection_ == LRDirection::kRight) ? kAnchorSpeed * 0.7071f : -kAnchorSpeed * 0.7071f; // cos(45)
		initialVelocity.y = -kAnchorSpeed * 0.7071f; // sin(45)
	} else if (lStickY < -kAnchorDeadZone || Input::GetInstance()->IsPress(DIK_W)) {
		// 下に傾いている場合は45度下向き
		xVelocity = (lrDirection_ == LRDirection::kRight) ? kAnchorSpeed * 0.7071f : -kAnchorSpeed * 0.7071f; // cos(45)
		initialVelocity.y = kAnchorSpeed * 0.7071f; // -sin(45)
	} else {
		// 上下に傾いていない場合は水平
		initialVelocity.y = 0.0f;
	}

	initialVelocity.x = xVelocity;
	initialVelocity.z = 0.0f;

	Vector3 spawnPos = { worldTransform_.translate.x, worldTransform_.translate.y,0.0f };
	anchor_ = std::make_unique<Anchor>(spawnPos, initialVelocity, mapChipField_);
}

Vector3 Player::GetWorldPosition() {
	return worldTransform_.translate;
}


AABB Player::GetAABB() {
	AABB aabb;
	Vector3 worldPos = GetWorldPosition();
	aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
	aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };
	return aabb;
}

void Player::OnCollision(Enemy* enemy) {
	// ノックバック中は再ノックバックさせない
	if (isKnockedBack_) {
		return;
	}

	// 速度をゼロにリセット
	velocity_ = { 0.0f, 0.0f, 0.0f };

	// 敵とプレイヤーの相対位置を計算
	Vector3 toEnemy = Subtract(enemy->GetWorldPosition(), worldTransform_.translate);

	// 水平方向のみ正規化して方向を求める
	toEnemy.y = 0.0f;
	toEnemy = Normalize(toEnemy);

	// プレイヤーが敵に当たったときに受ける衝撃ベクトルを計算
	Vector3 impactVector = {};
	impactVector.x = -toEnemy.x * kKnockbackPower; // 敵と逆方向に飛ばす
	impactVector.y = kKnockbackUpPower; // 上方向に少し浮かせる

	// プレイヤーの速度に衝撃ベクトルを加算
	velocity_ = Add(velocity_, impactVector);

	// ノックバック状態を開始
	isKnockedBack_ = true;
	knockbackTimer_ = kKnockbackTime;
}

// Player::RemoveLockedOnEnemiesメソッドの実装
void Player::RemoveLockedOnEnemies(std::list<Enemy*>& enemies) {
	for (Enemy* enemy : enemies) {
		enemy->SetIsDead(true); // 敵の死亡フラグを立てる
	}
	enemies.clear(); // ロックオンリストをクリア
}

void Player::HandleLockOnRemovalInput() {
	// リストへのポインタが設定されているか確認
	if (lockedOnEnemies_ == nullptr) {
		return;
	}

	// Lキーまたは対応するパッドボタンが押されたら、ロックオン中の敵をすべて削除する
	// GameSceneで使われていた入力判定をそのまま使用
	if (Input::GetInstance()->IsPadTrigger(3) || Input::GetInstance()->IsTrigger(DIK_L)) {
		// GameScene が持っていたリストへのポインタを介して、削除ロジックを呼び出す
		RemoveLockedOnEnemies(*lockedOnEnemies_);
	}
}