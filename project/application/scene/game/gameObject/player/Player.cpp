#define NOMINMAX
#include "Player.h"
#include "../enemy/Enemy.h"
#include "ImGuiManager.h"
#include"../../mapchip/MapChipField.h"
#include "../../logic/Collision.h"
#include <algorithm>
#include <numbers>
#include <cmath>

void Player::Initialize(Object3D* model, Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;

	worldTransform_ = {
		{1,1,1},
		{0,0,0},
		position,
	};

	camera_ = camera;

	onGround_ = false;

	width_ = kWidth;
	height_ = kHeight;

	hp_ = 3;

	anchorLine_ = std::make_unique<Bonjin::Line3D>();
	anchorLine_->Initialize();
}

void Player::Move() {
	if (isKnockedBack_) {
		return;
	}
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
	}
}

void Player::Update() {

	HandleLockOnRemovalInput();
	const bool wasOnGround = onGround_;


	// 1.移動処理
	Move();

	if (isKnockedBack_) {
		velocity_.x *= kKnockbackAttenuation;
		knockbackTimer_ -= 1.0f / 60.0f; // 1秒間に60フレームを想定
		if (knockbackTimer_ <= 0.0f) {
			isKnockedBack_ = false;
			knockbackTimer_ = 0.0f;
		}
	}

	if (isInvincible_) {
		invincibleTimer_ -= 1.0f / 60.0f; // 1秒間に60フレームを想定
		if (invincibleTimer_ <= 0.0f) {
			isInvincible_ = false;
			invincibleTimer_ = 0.0f;
		}
	}

	// 2.物理とマップ衝突判定の更新
	UpdatePhysicsAndMapCollision();

	if (!wasOnGround && onGround_) {
		landingEffectRequested_ = true;
	}

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
		if (!isKnockedBack_) {
			shootAnchor();
		}
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

	// アンカーとプレイヤーをつなぐラインの更新
	if (anchor_ != nullptr) {

		anchorLine_->Update(GetPosition(), anchor_->GetPosition(), camera_, lineColor_);
	}

	// Yボタンでテレポート
	if (Input::GetInstance()->IsPadTrigger(1) || Input::GetInstance()->IsTrigger(DIK_K)) {
		if (!isKnockedBack_) {
			// アンカーが存在し、isStandByがtrueの場合
			if (anchor_ && anchor_->GetStandBy()) {
				// アンカーの座標からテレポート先座標を取得
				Vector3 anchorPos = anchor_->GetPosition();
				Vector3 teleportPosition = anchorPos;

				//// キャラクターの高さの半分だけ上方向に移動
				//teleportPosition.y = anchorPos.y + kHeight / 2.0f;
				//// === ここまで修正 ===

				Vector3 anchorVelocity = anchor_->GetVelocity();

				// 体が埋まらないようにX座標を調整
				if (anchorVelocity.x > 0.0f) {
					teleportPosition.x -= kWidth / 2.0f;
				} else if (anchorVelocity.x < 0.0f) {
					teleportPosition.x += kWidth / 2.0f;
				}
				// 体が埋まらないようにY座標を調整
				if (anchorVelocity.y > 0.0f) {
					teleportPosition.y -= kHeight / 2.0f;
				} else if (anchorVelocity.y < 0.0f) {
					teleportPosition.y += kHeight / 2.0f;
				}

				// プレイヤーを計算された座標にテレポート
				worldTransform_.translate = teleportPosition;

				// アンカーを消去
				anchor_ = nullptr;
			}
		}
	}

}

void Player::Draw() {
	// 無敵時間中の点滅処理 (0.1秒周期)
	bool drawPlayerModel = true;
	if (isInvincible_) {
		if (std::fmod(invincibleTimer_, 0.1f) < 0.05f) {
			drawPlayerModel = false;
		}
	}

	// 自キャラの描画処理
	if (drawPlayerModel) {
		model_->Draw();
	}

	if (anchor_ != nullptr) 
	{
		anchor_->Draw();
	}
}

void Player::DrawAnchorLine() {
	if (anchor_ != nullptr) {
		anchorLine_->Draw();
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
	if (isInvincible_|| hp_<=0) {
		return;
	}

	isKnockedBack_ = true;
	knockbackTimer_ = kKnockbackTime;

	isInvincible_ = true;
	invincibleTimer_ = kInvincibleTime;

	ApplyDamage(1);

	// ノックバック方向を決定 (敵とプレイヤーの相対位置)
	Vector3 relative = Subtract(GetWorldPosition(), enemy->GetWorldPosition());
	float knockbackDirX = (relative.x >= 0.0f) ? 1.0f : -1.0f;

	// 速度を設定して弾き飛ばす
	velocity_.x = knockbackDirX * kKnockbackPower;
	velocity_.y = kKnockbackUpPower;
}

bool Player::ConsumeLandingEffectRequest() {
	if (!landingEffectRequested_) {
		return false;
	}

	landingEffectRequested_ = false;
	return true;
}

void Player::RemoveLockedOnEnemies(std::list<Enemy*>& enemies) {
	for (Enemy* enemy : enemies) {
		if (enemy != nullptr) {
			enemy->SetIsDead(true);
			enemy->SetIsLockedOn(false);
		}
	}
	enemies.clear();
}

void Player::HandleLockOnRemovalInput() {
	// リストへのポインタが設定されているか確認
	if (lockedOnEnemies_ == nullptr) {
		return;
	}

	// Lキーまたは対応するパッドボタンが押されたら、ロックオン中の敵をすべて削除する
	// GameSceneで使われていた入力判定をそのまま使用
	if (Input::GetInstance()->IsPadTrigger(3) || Input::GetInstance()->IsTrigger(DIK_L)) {
		RemoveLockedOnEnemies(*lockedOnEnemies_);
	}
}

void Player::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	BaseCharacter::OnMapCollision(collisionMapinfo);

	if (collisionMapinfo.isHitWall_) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

void Player::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Player")) {
		Vector3 pos = GetPosition();
		ImGui::Text("Player Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
		ImGui::Text("On Ground: %s", onGround_ ? "true" : "false");
		ImGui::InputInt("Player HP", &hp_);
		bool hasAnchor = HasAnchor();
		ImGui::Text("Has Anchor: %s", hasAnchor ? "true" : "false");

		if (hasAnchor) {
			Anchor& anchor = GetAnchor();
			Vector3 anchorPos = anchor.GetPosition();
			ImGui::Text("Anchor Position: (%.2f, %.2f, %.2f)", anchorPos.x, anchorPos.y, anchorPos.z);

			ImGui::ColorEdit4("Anchor (Line) Color", &lineColor_.x);

			if (ImGui::Button("Force Delete Anchor")) {
				anchor_ = nullptr;
			}
		}
		ImGui::TreePop();
	}
#endif
}
