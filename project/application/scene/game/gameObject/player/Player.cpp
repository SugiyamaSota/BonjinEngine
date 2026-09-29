#define NOMINMAX
#include "Player.h"
#include "input/Gamepad.h"
#include "Lightning3D.h"
#include "../enemy/BaseEnemy.h"
#include "SceneManager.h"
#include "ImGuiManager.h"
#include "ParticleManager.h"
#include <algorithm>
#include <numbers>
#include <cmath>

using namespace Bonjin;

void Player::Initialize(Object3D* model, Camera* camera, const Vector3& position) {
	width_ = kWidth;
	height_ = kHeight;

	GameObjectInitConfig config;
	config.physicsType = PhysicsType::Gravity;
	config.hasCollider = true;
	config.categoryAttr = kAttributePlayer;
	config.collisionMask = kAttributeEnemy | kAttributeGoal | kAttributeEnemyBullet;
	config.tag = "Player";
	GameObject::Initialize(model, camera, position, config);

	// プレイヤーステータスの初期化
	statusComponent_.Initialize();

	anchorLine_ = std::make_unique<Bonjin::Line3D>();
	anchorLine_->Initialize();

	isGoalReached_ = false;

	lightningEffect_ = std::make_unique<Bonjin::Lightning3D>();
	lightningEffect_->Initialize();

	frayLineEffect_ = std::make_unique<Bonjin::FrayLine3D>();
	frayLineEffect_->Initialize();

	retractAnchorModel_ = std::make_unique<Object3D>();
	retractAnchorModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	retractAnchorModel_->SetBlendMode(BlendMode::kNormal);
	retractAnchorModel_->SetEnableEnableEnvironmentMap(false);
}

void Player::OnCollision(Bonjin::Collider* other) {
	if (other->GetCategoryAttr() == kAttributeEnemy) {
		Bonjin::BaseEnemy* enemy = static_cast<Bonjin::BaseEnemy*>(other->GetOwner());
		OnCollision(enemy);
	} else if (other->GetCategoryAttr() == kAttributeGoal) {
		isGoalReached_ = true;
	}
}

void Player::Move() {
	if (isKnockedBack_ || !teleportQueue_.empty()) {
		return;
	}
	// 移動入力
	// 接地状態
	if (onGround_) {
		// ゲームパッドの左スティックのX軸の値を取得
		long lStickX = Gamepad::GetInstance()->GetLStickX();

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
		if (Gamepad::GetInstance()->IsPress(XINPUT_GAMEPAD_A) || Input::GetInstance()->IsTrigger(DIK_SPACE)) {
			velocity_ = Add(velocity_, Vector3(0, kJumpAcceleration, 0));
		}
	} else {
		// 空中
		// === ここから空中での移動処理を修正 ===
		long lStickX = Gamepad::GetInstance()->GetLStickX();

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
	// ダメージVignette演出の更新
	if (damageEffectTimer_ > 0.0f) {
		damageEffectTimer_ -= 1.0f / 60.0f;
		auto sceneManager = Bonjin::SceneManager::GetInstance();
		if (damageEffectTimer_ <= 0.0f) {
			damageEffectTimer_ = 0.0f;
			sceneManager->SetFullScreenVignette(wasVignette_);
			sceneManager->SetFullScreenVignetteColor(origVignetteColor_);
			sceneManager->SetFullScreenVignetteScale(origVignetteScale_);
			sceneManager->SetFullScreenVignettePower(origVignettePower_);
		} else {
			float t = 1.0f - (damageEffectTimer_ / kDamageEffectMaxTime);
			// EaseOutQuad を使って一瞬で画面が赤くなり、最初は速く、後からゆっくり消えるようにする
			float easedT = 1.0f - (1.0f - t) * (1.0f - t);

			float targetScale = (origVignetteScale_ > 24.0f) ? origVignetteScale_ : 24.0f;
			float initialScale = 3.0f;
			float currentScaleVal = initialScale + easedT * (targetScale - initialScale);
			sceneManager->SetFullScreenVignetteScale(currentScaleVal);

			Vector3 redColor = { 1.0f, 0.0f, 0.0f };
			Vector3 currentVigColor = {
				origVignetteColor_.x + (1.0f - easedT) * (redColor.x - origVignetteColor_.x),
				origVignetteColor_.y + (1.0f - easedT) * (redColor.y - origVignetteColor_.y),
				origVignetteColor_.z + (1.0f - easedT) * (redColor.z - origVignetteColor_.z)
			};
		}
	}

	// テレポートブラー演出の更新
	if (teleportBlurTimer_ > 0.0f) {
		teleportBlurTimer_ -= 1.0f / 60.0f;
		auto sceneManager = Bonjin::SceneManager::GetInstance();
		if (teleportBlurTimer_ <= 0.0f) {
			teleportBlurTimer_ = 0.0f;
			sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kRadialBlur);
			sceneManager->SetRadialBlurWidth(0.0f);
		} else {
			auto activeEffects = sceneManager->GetActiveEffects();
			if (std::find(activeEffects.begin(), activeEffects.end(), DirectXCommon::PostEffect::kRadialBlur) == activeEffects.end()) {
				sceneManager->AddPostEffect(DirectXCommon::PostEffect::kRadialBlur);
			}
			float t = teleportBlurTimer_ / kTeleportBlurMaxTime; // 1.0 -> 0.0
			float easedT = t * t;
			sceneManager->SetRadialBlurWidth(easedT * 0.025f);
		}
	}

	HandleLockOnRemovalInput();
	const bool wasOnGround = onGround_;

	// テレポートキューの更新
	if (!teleportQueue_.empty()) {
		teleportTimer_ -= 1.0f / 60.0f;
		if (teleportTimer_ <= 0.0f) {
			lastTeleportStartPos_ = worldTransform_.translate; // テレポート元の位置を保存

			worldTransform_.translate = teleportQueue_.front();
			teleportQueue_.pop_front();
			teleportTimer_ = teleportInterval_;

			lightningActiveTimer_ = lightningShowDuration_; // 雷を発生させる
			teleportBlurTimer_ = kTeleportBlurMaxTime;      // テレポートブラーを開始

			velocity_ = { 0.0f, 0.0f, 0.0f }; // 速度のリセット

			// カメラシェイクとヒットストップ
			if (camera_) {
				camera_->StartShake(2.f,1.f);
			}
			hitStopRequested_ = true;
		}
	}

	// 雷タイマーの更新
	if (lightningActiveTimer_ > 0.0f) {
		lightningActiveTimer_ -= 1.0f / 60.0f;
	}

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
	GameObject::Update();

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

	// アンカーの時間経過リチャージ処理
	if (anchorStock_ < kMaxAnchorStock) {
		float rechargeDuration = GetAnchorRechargeDuration();
		if (rechargeDuration > 0.0f) {
			anchorRechargeTimer_ += 1.0f / 60.0f;
			if (anchorRechargeTimer_ >= rechargeDuration) {
				anchorStock_++;
				anchorRechargeTimer_ = 0.0f;
			}
		}
	} else {
		anchorRechargeTimer_ = 0.0f;
	}

	// アンカー操作（発射 / 長押し回収）
	bool isAnchorKeyPressed = Gamepad::GetInstance()->IsPress(XINPUT_GAMEPAD_X) || Input::GetInstance()->IsPress(DIK_J);
	bool isAnchorKeyTriggered = Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_X) || Input::GetInstance()->IsTrigger(DIK_J);

	if (anchor_ != nullptr) {
		// すでにアンカーが存在する場合：長押しで回収
		if (isAnchorKeyPressed) {
			anchorHoldTimer_ += 1.0f / 60.0f;
			if (anchorHoldTimer_ >= kAnchorHoldTime) {
				// ほつれ切断エフェクト開始
				isFrayActive_ = true;
				frayTimer_ = 0.0f;
				lastAnchorRetractPos_ = anchor_->GetPosition();

				retractAnchorTransform_.translate = lastAnchorRetractPos_;
				retractAnchorTransform_.scale = { 1.0f, 1.0f, 1.0f };
				retractAnchorTransform_.rotate = { 0.0f, 0.0f, anchor_->GetAngle() };

				anchor_ = nullptr; // アンカーを削除（回収）
				anchorHoldTimer_ = 0.0f;
			}
		} else {
			anchorHoldTimer_ = 0.0f;
		}
	} else {
		// アンカーが存在しない場合：トリガーで発射
		anchorHoldTimer_ = 0.0f;
		if (isAnchorKeyTriggered) {
			if (!isKnockedBack_) {
				shootAnchor();
			}
		}
	}

	// ほつれ切断エフェクトの進行
	if (isFrayActive_) {
		frayTimer_ += 1.0f / 60.0f;
		float progress = (std::clamp)(frayTimer_ / kFrayDuration, 0.0f, 1.0f);

		// 重力による自然な落下計算（初速ゼロから下へ加速落下）
		float dropDistance = progress * progress * 0.9f;
		retractAnchorTransform_.translate.x = lastAnchorRetractPos_.x;
		retractAnchorTransform_.translate.y = lastAnchorRetractPos_.y - dropDistance;
		retractAnchorTransform_.translate.z = lastAnchorRetractPos_.z;
		retractAnchorTransform_.rotate.z += 0.04f;

		// ほつれ糸の更新（落下するアンカー位置に連動）
		if (frayLineEffect_) {
			frayLineEffect_->Update(GetPosition(), retractAnchorTransform_.translate, camera_, lineColor_, progress, 0.0f);
		}

		// アンカー本体のゆっくりフェードアウト処理（落下＋縮小＋透明化）
		if (retractAnchorModel_) {
			float fadeAlpha = (std::max)(0.0f, 1.0f - progress * progress);
			float scale = (std::max)(0.0f, 1.0f - progress * 0.4f);
			retractAnchorTransform_.scale = { scale, scale, scale };
			retractAnchorModel_->SetColor({ 0.0f, 0.0f, 1.0f, fadeAlpha });
			retractAnchorModel_->Update(retractAnchorTransform_, camera_);
		}

		if (frayTimer_ >= kFrayDuration) {
			isFrayActive_ = false;
		}
	}
	// 長押しチャージ中（アンカーが存在し、長押しタイマー進行中）のテンション振動
	else if (anchor_ != nullptr && anchorHoldTimer_ > 0.0f) {
		float tension = anchorHoldTimer_ / kAnchorHoldTime;
		if (frayLineEffect_) {
			frayLineEffect_->Update(GetPosition(), anchor_->GetPosition(), camera_, lineColor_, 0.0f, tension);
		}
	}

	// アンカーの更新
	CollisionMapInfo info; // マップの衝突情報

	if (anchor_ != nullptr) {
		// アンカーを更新
		anchor_->Update();
		// 衝突フラグが立っているかチェック
		if (anchor_->IsDead()) {
			anchor_ = nullptr; // アンカーを削除
		}
	}

	// アンカーとプレイヤーをつなぐラインの更新
	if (anchor_ != nullptr) {

		anchorLine_->Update(GetPosition(), anchor_->GetPosition(), camera_, lineColor_);
	}

	// BボタンまたはKキーでテレポート
	if (Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_B) || Input::GetInstance()->IsTrigger(DIK_K)) {
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
	else if (isFrayActive_ && retractAnchorModel_)
	{
		// 回収中のアンカー本体フェードアウト描画
		retractAnchorModel_->Draw();
	}

	// 雷霆エフェクトの描画
	if (lightningActiveTimer_ > 0.0f && lightningEffect_) {
		lightningEffect_->Update(lastTeleportStartPos_, GetWorldPosition(), camera_, lightningColor_, lightningOffsetRatio_, lightningMinOffset_, lightningMaxOffsetLimit_);
		lightningEffect_->Draw();
	}
}

void Player::DrawAnchorLine() {
	if (isFrayActive_ || (anchor_ != nullptr && anchorHoldTimer_ > 0.0f)) {
		if (frayLineEffect_) {
			frayLineEffect_->Draw();
		}
	} else if (anchor_ != nullptr) {
		anchorLine_->Draw();
	}
}

void Player::shootAnchor() {
	// アンカーがすでに存在する場合、またはストックが0の場合は何もしない
	if (anchor_ != nullptr || anchorStock_ <= 0) {
		return;
	}

	// ストックを1消費
	anchorStock_--;

	// ゲームパッドの左スティックのX軸とY軸の値を取得
	long lStickX = Gamepad::GetInstance()->GetLStickX();
	long lStickY = Gamepad::GetInstance()->GetLStickY();

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
	anchor_ = std::make_unique<Anchor>(spawnPos, initialVelocity, mapChipField_, this, camera_);
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

void Player::OnCollision(Bonjin::BaseEnemy* enemy) {
	if (isInvincible_|| statusComponent_.GetHp() <= 0) {
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

void Player::GainExp(int amount) {
	if (statusComponent_.IsDead()) return;

	bool levelUp = statusComponent_.GainExp(amount);
	if (levelUp) {
		// レベルアップ時の演出（エフェクト生成やSE再生など）をここに記述
	}
}
void Player::RemoveLockedOnEnemies(std::list<Bonjin::BaseEnemy*>& enemies) {
	size_t defeatedCount = 0;
	for (Bonjin::BaseEnemy* enemy : enemies) {
		if (enemy != nullptr && !enemy->GetIsDead()) {
			// 敵の座標をテレポートキューに追加
			teleportQueue_.push_back(enemy->GetWorldPosition());

			GainExp(enemy->GetExpReward());
			enemy->SetIsDead(true);
			enemy->SetIsLockedOn(false);
			defeatedCount++;
		}
	}
	enemies.clear();

	// 撃破ボーナス: 2体以上の連続撃破で即座に1個回復、1体撃破でリチャージゲージ短縮
	if (defeatedCount >= 2) {
		RestoreAnchorStock(1);
	} else if (defeatedCount == 1) {
		AddAnchorRechargeProgress(1.0f);
	}

	// 最初のテレポートを開始
	if (!teleportQueue_.empty()) {
		lastTeleportStartPos_ = worldTransform_.translate; // 開始位置を保存

		worldTransform_.translate = teleportQueue_.front();
		teleportQueue_.pop_front();
		teleportTimer_ = teleportInterval_;

		lightningActiveTimer_ = lightningShowDuration_; // 雷を発生させる
		teleportBlurTimer_ = kTeleportBlurMaxTime;      // テレポートブラーを開始

		velocity_ = { 0.0f, 0.0f, 0.0f };

		// カメラシェイクとヒットストップ
		if (camera_) {
			camera_->StartShake(0.5f, 0.1f);
		}
		hitStopRequested_ = true;

		// 演出終了まで無敵状態にする
		isInvincible_ = true;
		invincibleTimer_ = teleportInterval_ * (teleportQueue_.size() + 2);
	}
}

void Player::AddAnchorRechargeProgress(float seconds) {
	if (anchorStock_ >= kMaxAnchorStock) {
		return;
	}
	anchorRechargeTimer_ += seconds;
	float rechargeDuration = GetAnchorRechargeDuration();
	while (anchorRechargeTimer_ >= rechargeDuration && anchorStock_ < kMaxAnchorStock) {
		anchorStock_++;
		anchorRechargeTimer_ -= rechargeDuration;
		if (anchorStock_ >= kMaxAnchorStock) {
			anchorRechargeTimer_ = 0.0f;
			break;
		}
	}
}

void Player::RestoreAnchorStock(int count) {
	anchorStock_ = (std::min)(kMaxAnchorStock, anchorStock_ + count);
	if (anchorStock_ >= kMaxAnchorStock) {
		anchorRechargeTimer_ = 0.0f;
	}
}

void Player::HandleLockOnRemovalInput() {
	// リストへのポインタが設定されているか確認
	if (lockedOnEnemies_ == nullptr) {
		return;
	}

	// Lキーまたは対応するパッドボタンが押されたら、ロックオン中の敵をすべて削除する
	// GameSceneで使われていた入力判定をそのまま使用
	if (Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_Y) || Input::GetInstance()->IsTrigger(DIK_L)) {
		RemoveLockedOnEnemies(*lockedOnEnemies_);
	}
}

void Player::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	GameObject::OnMapCollision(collisionMapinfo);

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
		ImGui::Text("Player HP: %d", statusComponent_.GetHp());
		ImGui::Text("Player Level: %d", statusComponent_.GetLevel());
		ImGui::Text("Player Exp: %d / %d", statusComponent_.GetExp(), statusComponent_.GetRequiredExp());
		ImGui::Text("Player Max HP: %d", statusComponent_.GetMaxHp());
		ImGui::Text("Player Attack Power: %d", statusComponent_.GetAttackPower());
		ImGui::Text("Anchor Stock: %d / %d (Recharge: %.2fs / %.2fs)", anchorStock_, kMaxAnchorStock, anchorRechargeTimer_, GetAnchorRechargeDuration());
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

		if (ImGui::TreeNode("Anchor Fray & Hold Retract")) {
			ImGui::Text("Hold Timer: %.2f / %.2f", anchorHoldTimer_, kAnchorHoldTime);
			ImGui::Text("Fray Active: %s (Timer: %.2f / %.2f)", isFrayActive_ ? "true" : "false", frayTimer_, kFrayDuration);
			ImGui::ColorEdit4("Line Color", &lineColor_.x);
			ImGui::TreePop();
		}

		// 雷霆テレポートエフェクトの調整UI
		if (ImGui::TreeNode("Teleport & Lightning Effect")) {
			ImGui::DragFloat("Teleport Interval", &teleportInterval_, 0.01f, 0.01f, 1.0f, "%.2fs");
			ImGui::DragFloat("Lightning Duration", &lightningShowDuration_, 0.01f, 0.01f, 1.0f, "%.2fs");
			ImGui::ColorEdit4("Lightning Color", &lightningColor_.x);
			ImGui::DragFloat("Lightning Offset Ratio", &lightningOffsetRatio_, 0.005f, 0.0f, 0.5f, "%.3f");
			ImGui::DragFloat("Lightning Min Offset", &lightningMinOffset_, 0.01f, 0.0f, 5.0f, "%.2f");
			ImGui::DragFloat("Lightning Max Offset", &lightningMaxOffsetLimit_, 0.01f, 0.0f, 10.0f, "%.2f");
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
#endif
}

void Player::EmitAnchorHitEffect(const Vector3& position) {
	static std::mt19937 randomEngine(std::random_device{}());
	ParticleManager* particleManager = ParticleManager::GetInstance();
	
	constexpr uint32_t kRayCount = 8;
	std::uniform_real_distribution<float> rotateDistribution(
		-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> lengthDistribution(1.2f, 2.2f);

	for (uint32_t index = 0; index < kRayCount; ++index) {
		ParticleConfig ray{};
		ray.position = position;
		ray.rotate = {0.0f, 0.0f, rotateDistribution(randomEngine)};
		ray.velocity = {0.0f, 0.0f, 0.0f};
		ray.scale = {0.12f, lengthDistribution(randomEngine), 1.0f};
		ray.color = {0.35f, 0.75f, 1.0f, 1.0f};
		ray.lifeTime = 0.28f;
		ray.updateFunc = [](ParticleData& particle, float deltaTime) {
			particle.transform.scale.y += 4.0f * deltaTime;
			particle.transform.scale.x -= 0.25f * deltaTime;
			if (particle.transform.scale.x < 0.0f) {
				particle.transform.scale.x = 0.0f;
			}
		};
		particleManager->Emit("anchorHitRay", ray);
	}

	ParticleConfig flash{};
	flash.position = position;
	flash.scale = {0.9f, 0.9f, 1.0f};
	flash.color = {0.65f, 0.9f, 1.0f, 1.0f};
	flash.lifeTime = 0.18f;
	flash.updateFunc = [](ParticleData& particle, float deltaTime) {
		const float expansion = 4.0f * deltaTime;
		particle.transform.scale.x += expansion;
		particle.transform.scale.y += expansion;
	};
	particleManager->Emit("anchorHitFlash", flash);
}

void Player::ApplyDamage(int damage) {
	statusComponent_.ApplyDamage(damage);

	// ダメージ演出の開始処理
	if (damageEffectTimer_ <= 0.0f) {
		auto sceneManager = Bonjin::SceneManager::GetInstance();
		wasVignette_ = sceneManager->IsFullScreenVignette();
		origVignetteColor_ = sceneManager->GetFullScreenVignetteColor();
		origVignetteScale_ = sceneManager->GetFullScreenVignetteScale();
		origVignettePower_ = sceneManager->GetFullScreenVignettePower();
	}

	damageEffectTimer_ = kDamageEffectMaxTime;
	auto sceneManager = Bonjin::SceneManager::GetInstance();
	sceneManager->SetFullScreenVignette(true);
	sceneManager->SetFullScreenVignetteColor({ 1.0f, 0.0f, 0.0f });
	sceneManager->SetFullScreenVignetteScale(3.0f);
}
