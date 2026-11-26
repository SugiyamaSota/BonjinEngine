#define NOMINMAX
#include "Player.h"
#include "input/Gamepad.h"
#include "GpuParticleManager.h"
#include "../enemy/BaseEnemy.h"
#include "SceneManager.h"
#include "ImGuiManager.h"
#include "ParticleManager.h"
#include "../../mapchip/MapChipField.h"
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

	frayLineEffect_ = std::make_unique<Bonjin::FrayLine3D>();
	frayLineEffect_->Initialize();

	// 回収中アンカーはプレイヤーモデルではなく球を使う
	retractAnchorModel_ = std::make_unique<Object3D>();
	retractAnchorModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	retractAnchorModel_->SetBlendMode(BlendMode::kNormal);
	retractAnchorModel_->SetEnableEnableEnvironmentMap(false);

	// スケルトン付きモデルならアニメーションを準備
	hasAnimator_ = model_ && model_->GetSkeleton().has_value();
	if (hasAnimator_) {
		animator_.Initialize(*model_->GetSkeleton());
	}
}

void Player::UpdateModel() {
	if (!model_) {
		return;
	}
	// モデルは足元原点・高さ約1.0なので、当たり判定(中心原点・高さkHeight)に合わせて補正する
	if (hasAnimator_) {
		animator_.Apply(*model_->GetSkeleton());
	}
	model_->Update(GetModelWorldTransform(), camera_);
}

WorldTransform Player::GetModelBaseWorldTransform() const {
	// 演出用の倍率を掛ける前のモデルの配置 (足元を当たり判定の底に合わせる・向き補正)
	WorldTransform modelTransform = worldTransform_;
	modelTransform.scale = { modelScale_, modelScale_, modelScale_ };
	modelTransform.rotate.y += modelRotateYOffset_;
	modelTransform.translate.y += modelOffsetY_;
	return modelTransform;
}

WorldTransform Player::GetModelWorldTransform() const {
	WorldTransform modelTransform = GetModelBaseWorldTransform();
	if (visualScaleMultiplier_ == 1.0f) {
		return modelTransform;
	}
	// 演出用の倍率は見た目だけ。体の中央(腰)を基準点にして大きさを変える。
	// 原点(足元)をOとすると、基準点Pまわりにk倍した原点は P + k*(O - P)
	const float k = visualScaleMultiplier_;
	const Vector3 pivot = GetModelCenterPosition(); // 倍率を掛ける前の腰の位置 (基準点なので倍率を掛けても動かない)
	const Vector3 origin = modelTransform.translate;
	modelTransform.translate = {
		pivot.x + k * (origin.x - pivot.x),
		pivot.y + k * (origin.y - pivot.y),
		pivot.z + k * (origin.z - pivot.z),
	};
	const float scale = modelScale_ * k;
	modelTransform.scale = { scale, scale, scale };
	return modelTransform;
}

Vector3 Player::GetModelCenterPosition() const {
	// アニメーション後の腰(Hips)のワールド座標。Flair中などは体が当たり判定の中心より低くなるため
	// (演出の拡大縮小は腰を基準点にしているので、倍率を掛ける前の配置で求めても同じ位置になる)
	if (model_ && model_->GetSkeleton().has_value()) {
		const Skeleton& skeleton = *model_->GetSkeleton();
		auto it = skeleton.jointMap.find("mixamorig:Hips");
		if (it != skeleton.jointMap.end()) {
			const Matrix4x4& joint = skeleton.joints[it->second].skeletonSpaceMatrix;
			const WorldTransform t = GetModelBaseWorldTransform();
			const Matrix4x4 world = MakeAffineMatrix(t.scale, t.rotate, t.translate);
			const float x = joint.m[3][0], y = joint.m[3][1], z = joint.m[3][2];
			return {
				x * world.m[0][0] + y * world.m[1][0] + z * world.m[2][0] + world.m[3][0],
				x * world.m[0][1] + y * world.m[1][1] + z * world.m[2][1] + world.m[3][1],
				x * world.m[0][2] + y * world.m[1][2] + z * world.m[2][2] + world.m[3][2],
			};
		}
	}
	return worldTransform_.translate;
}

void Player::ResolveMapEmbedding() {
	if (!mapChipField_) {
		return;
	}
	constexpr float kInset = 0.01f;
	auto isBlock = [&](float x, float y) {
		const IndexSet index = mapChipField_->GetMapChipIndexSetByPosition({ x, y, 0.0f });
		return mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipType::kBlock;
	};
	auto rectAt = [&](float x, float y) {
		const IndexSet index = mapChipField_->GetMapChipIndexSetByPosition({ x, y, 0.0f });
		return mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);
	};

	// 上下に押し出すと別のブロックに当たることもあるので数回繰り返す
	for (int i = 0; i < 4; ++i) {
		Vector3& pos = worldTransform_.translate;
		const float leftX = pos.x - width_ / 2.0f + kInset;
		const float rightX = pos.x + width_ / 2.0f - kInset;
		const float bottomY = pos.y - height_ / 2.0f + kInset;
		const float topY = pos.y + height_ / 2.0f - kInset;

		// 足元がめり込んでいたら、そのブロックの上面に乗せる
		if (isBlock(leftX, bottomY) || isBlock(rightX, bottomY)) {
			const float x = isBlock(leftX, bottomY) ? leftX : rightX;
			pos.y = rectAt(x, bottomY).top + height_ / 2.0f;
			continue;
		}
		// 頭がめり込んでいたら、そのブロックの下面まで下げる
		if (isBlock(leftX, topY) || isBlock(rightX, topY)) {
			const float x = isBlock(leftX, topY) ? leftX : rightX;
			pos.y = rectAt(x, topY).bottom - height_ / 2.0f;
			continue;
		}
		break;
	}
}

void Player::StartClearPerformance() {
	if (clearPerformance_.IsActive()) {
		return;
	}
	clearPerformance_.Start();

	// 演出の邪魔になるものを止める
	anchor_ = nullptr;
	isFrayActive_ = false;
	teleportQueue_.clear();
	isGravitySuspended_ = false;
	isKnockedBack_ = false;
	isInvincible_ = false; // 点滅で消えたまま止まらないように
	visualScaleMultiplier_ = 1.0f;
	invincibleTimer_ = 0.0f;

	// 振り返り途中なら向きを確定させる
	if (turnTimer_ > 0.0f) {
		turnTimer_ = 0.0f;
		worldTransform_.rotate.y = (lrDirection_ == LRDirection::kRight) ? 0.0f : std::numbers::pi_v<float>;
	}
}

void Player::UpdateClearPerformance() {
	if (!clearPerformance_.IsActive()) {
		return;
	}
	const float deltaTime = 1.0f / 60.0f;

	// 入力は受け付けず、横の勢いだけ止めて重力と地形判定は続ける
	velocity_.x *= 0.8f;
	if (std::abs(velocity_.x) < 0.001f) {
		velocity_.x = 0.0f;
	}

	// 演出の段階を進める (段階ごとの処理は PlayerClearPerformance の各段階のクラスが持つ)
	ClearPerformanceInput input;
	input.isOnGround = onGround_;
	input.flairStartLength = GetAnimationPlayLength(PlayerAnimator::State::kFlairStart);
	input.flairingLength = GetAnimationPlayLength(PlayerAnimator::State::kFlairing);
	input.flairFinishLength = GetAnimationPlayLength(PlayerAnimator::State::kFlairFinish);
	clearPerformance_.Update(input, deltaTime);

	visualScaleMultiplier_ = clearPerformance_.GetVisualScale();
	if (clearPerformance_.ConsumeVanishLightningRequest()) {
		const Vector3 center = GetModelCenterPosition();
		const float height = clearPerformance_.GetSettings().vanishLightningHeight;
		EmitTeleportLightning(center, { center.x, center.y + height, center.z });
	}

	if (hasAnimator_) {
		animator_.Update(deltaTime, clearPerformance_.GetAnimation());
	} else if (clearPerformance_.GetPhase() != ClearPhase::kLanding) {
		clearPerformance_.Skip(); // アニメーションが無ければ即終了
	}

	// 物理・地形判定とモデル更新
	GameObject::Update();
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
	// クリア演出中は演出の更新だけ行う
	if (clearPerformance_.IsActive()) {
		// ImGuiのテスト再生が終わったら通常操作に戻す (本番はゴール後なのでGoalフェーズ側が遷移させる)
		if (clearPerformance_.IsFinished() && !isGoalReached_) {
			clearPerformance_.Stop();
			visualScaleMultiplier_ = 1.0f; // テストで消えた見た目を元に戻す
		} else {
			UpdateClearPerformance();
			return;
		}
	}

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
			sceneManager->RemovePostEffect(PostEffect::kRadialBlur);
			sceneManager->SetRadialBlurWidth(0.0f);
		} else {
			auto activeEffects = sceneManager->GetActiveEffects();
			if (std::find(activeEffects.begin(), activeEffects.end(), PostEffect::kRadialBlur) == activeEffects.end()) {
				sceneManager->AddPostEffect(PostEffect::kRadialBlur);
			}
			float t = teleportBlurTimer_ / (std::max)(0.001f, teleportBlurMaxTime_); // 1.0 -> 0.0
			float easedT = t * t;
			sceneManager->SetRadialBlurWidth(easedT * teleportBlurWidth_);
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
			ResolveMapEmbedding(); // 背が高いので床・天井にめり込んだら押し出す
			UpdateModel(); // テレポート先の位置で即座にモデル行列を更新

			// GPU雷霆エフェクトの発生
			EmitTeleportLightning(lastTeleportStartPos_, worldTransform_.translate);
			if (enableTeleportBlur_) {
				teleportBlurTimer_ = teleportBlurMaxTime_; // テレポートブラーを開始
			}

			StartPostTeleportFloat(); // 速度リセット＋滞空開始

			// カメラシェイク
			if (enableTeleportShake_ && camera_) {
				camera_->StartShake(lockonTeleportShakeIntensity_, lockonTeleportShakeDuration_);
			}

			// テレポート終了時（最後の敵）はフィニッシュヒットストップ、途中は短いヒットストップ
			if (teleportQueue_.empty()) {
				RequestHitStop(teleportFinishHitStopDuration_);
			} else {
				RequestHitStop(teleportMidHitStopDuration_);
			}
		}
	}

	// 1.移動処理
	Move();

	// テレポート中・テレポート直後は重力を止めて滞空させる（急落下防止）
	if (postTeleportFloatTimer_ > 0.0f) {
		postTeleportFloatTimer_ -= 1.0f / 60.0f;
	}
	isGravitySuspended_ = !teleportQueue_.empty() || postTeleportFloatTimer_ > 0.0f;
	if (isGravitySuspended_ && velocity_.y < 0.0f) {
		velocity_.y = 0.0f;
	}

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

	// アニメーション状態の更新 (空中:ジャンプ / 地上で移動中:走り / それ以外:待機)
	if (hasAnimator_) {
		PlayerAnimator::State animState = PlayerAnimator::State::kIdle;
		if (!onGround_) {
			animState = PlayerAnimator::State::kJump;
		} else if (std::abs(velocity_.x) > runAnimationSpeedThreshold_) {
			animState = PlayerAnimator::State::kRun;
		}
		animator_.Update(1.0f / 60.0f, animState);
	}

	// 2.物理とマップ衝突判定の更新
	GameObject::Update();

	if (!wasOnGround && onGround_) {
		landingEffectRequested_ = true;
	}

	// 旋回制御
	if (turnTimer_ > 0.0f) {

		{
			turnTimer_ = (std::max)(turnTimer_ - 1.0f / 60.0f, 0.0f);
			// 左右の自キャラ角度テーブル (右:0, 左:π)
			// 0とπの間を補間するので、振り返り中は必ずカメラ側を向いて回る
			float destinationRotationYTable[] = {
				0.0f,
				std::numbers::pi_v<float>,
			};
			// 状況に応じた角度を取得する
			float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
			// 旋回開始角度から目標角度へ補間する (0→1)
			float t = 1.0f - turnTimer_ / kTimeTurn;
			t = t * t * (3.0f - 2.0f * t); // smoothstep
			worldTransform_.rotate.y = turnFirstRotationY_ + (destinationRotationY - turnFirstRotationY_) * t;
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

	// ストックが増えた瞬間に光沢リングを出す（リチャージ / 撃破回復など全経路を検知）
	UpdateStockGainEffect();

	// ストック0の間はエンジンがショートしたような火花を散らす
	UpdateShortCircuitEffect();

	// アンカー操作（発射 / 長押し回収）
	bool isAnchorKeyPressed = Gamepad::GetInstance()->IsPress(XINPUT_GAMEPAD_X) || Input::GetInstance()->IsPress(DIK_J);
	bool isAnchorKeyTriggered = Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_X) || Input::GetInstance()->IsTrigger(DIK_J);

	// 頭上のストック表示を要求（ストック切れで撃てない場合は拒否演出）
	if (isAnchorKeyTriggered) {
		anchorIndicatorRequested_ = true;
		anchorShootDenied_ = (anchor_ == nullptr && anchorStock_ <= 0);
	}

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

				Vector3 startPos = worldTransform_.translate;
				// プレイヤーを計算された座標にテレポート
				worldTransform_.translate = teleportPosition;
				ResolveMapEmbedding(); // 背が高いので床・天井にめり込んだら押し出す
				UpdateModel(); // テレポート先の位置で即座にモデル行列を更新
				StartPostTeleportFloat(); // 速度リセット＋滞空開始

				// GPU雷霆テレポートエフェクトとブラーの発生
				EmitTeleportLightning(startPos, teleportPosition);
				if (enableTeleportBlur_) {
					teleportBlurTimer_ = teleportBlurMaxTime_;
				}
				if (enableTeleportShake_ && camera_) {
					camera_->StartShake(teleportShakeIntensity_, teleportShakeDuration_);
				}

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

	// 撃破ボーナス: 敵を倒したらアンカーを回復（倒した敵1体につき1個回復）
	if (defeatedCount > 0) {
		RestoreAnchorStock(static_cast<int>(defeatedCount));
	}

	// 最初のテレポートを開始
	if (!teleportQueue_.empty()) {
		lastTeleportStartPos_ = worldTransform_.translate; // 開始位置を保存

		worldTransform_.translate = teleportQueue_.front();
		teleportQueue_.pop_front();
		teleportTimer_ = teleportInterval_;
		ResolveMapEmbedding(); // 背が高いので床・天井にめり込んだら押し出す
		UpdateModel(); // テレポート先の位置で即座にモデル行列を更新

		// GPU雷霆エフェクトの発生
		EmitTeleportLightning(lastTeleportStartPos_, worldTransform_.translate);
		if (enableTeleportBlur_) {
			teleportBlurTimer_ = teleportBlurMaxTime_; // テレポートブラーを開始
		}

		StartPostTeleportFloat(); // 速度リセット＋滞空開始

		// カメラシェイク
		if (enableTeleportShake_ && camera_) {
			camera_->StartShake(lockonTeleportShakeIntensity_, lockonTeleportShakeDuration_);
		}

		// 初回テレポート（敵が1体のみなら終了時フィニッシュ、まだ残っていれば途中）
		if (teleportQueue_.empty()) {
			RequestHitStop(teleportFinishHitStopDuration_);
		} else {
			RequestHitStop(teleportMidHitStopDuration_);
		}

		// 演出終了まで無敵状態にする
		isInvincible_ = true;
		invincibleTimer_ = teleportInterval_ * (teleportQueue_.size() + 2);
	}
}

void Player::StartPostTeleportFloat() {
	velocity_ = { 0.0f, 0.0f, 0.0f };
	postTeleportFloatTimer_ = postTeleportFloatDuration_;
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
		ImGui::DragFloat("Model Scale", &modelScale_, 0.01f, 0.01f, 10.0f);
		ImGui::DragFloat("Model Offset Y", &modelOffsetY_, 0.01f);
		ImGui::SliderAngle("Model Rotate Y Offset", &modelRotateYOffset_);
		ImGui::DragFloat("Run Anim Threshold", &runAnimationSpeedThreshold_, 0.001f, 0.0f, 1.0f);
		ClearPerformanceSettings& clearSettings = clearPerformance_.GetSettings();
		ImGui::SliderInt("Clear Flair Loops", &clearSettings.flairLoopCount, 0, 10);
		if (ImGui::TreeNode("Clear Vanish (決めポーズ後に消える)")) {
			ImGui::DragFloat("Pose Hold Time", &clearSettings.poseHoldTime, 0.01f, 0.0f, 3.0f, "%.2f s");
			ImGui::DragFloat("Grow Time", &clearSettings.vanishGrowTime, 0.01f, 0.0f, 2.0f, "%.2f s");
			ImGui::DragFloat("Grow Scale", &clearSettings.vanishGrowScale, 0.01f, 1.0f, 3.0f);
			ImGui::DragFloat("Shrink Time", &clearSettings.vanishShrinkTime, 0.01f, 0.0f, 2.0f, "%.2f s");
			ImGui::Checkbox("Lightning On Vanish", &clearSettings.vanishLightning);
			ImGui::DragFloat("Lightning Height", &clearSettings.vanishLightningHeight, 0.1f, 0.0f, 50.0f);
			ImGui::Text("Visual Scale: %.2f", visualScaleMultiplier_);
			ImGui::TreePop();
		}
		if (ImGui::Button("Test Clear Performance")) {
			clearPerformance_.Stop();
			StartClearPerformance();
		}
		if (hasAnimator_) {
			animator_.DrawImGui();
		}
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

		if (ImGui::TreeNode("Anchor Stock Gain Ring")) {
			ImGui::Checkbox("Enable", &enableStockGainRing_);
			ImGui::ColorEdit4("Color", &stockGainRingColor_.x);
			ImGui::DragFloat("Scale", &stockGainRingScale_, 0.05f, 0.1f, 10.0f);
			ImGui::DragFloat("Life Time", &stockGainRingLifeTime_, 0.01f, 0.05f, 3.0f, "%.2fs");
			ImGui::DragFloat("Intensity", &stockGainRingIntensity_, 0.05f, 0.0f, 10.0f);
			ImGui::DragFloat("Highlight Spin", &stockGainRingSpin_, 0.05f, -20.0f, 20.0f, "%.2f rad/s");
			if (ImGui::Button("Test Emit")) {
				prevAnchorStockForFx_ = anchorStock_ - 1;
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Anchor Empty Short Circuit")) {
			ImGui::Checkbox("Enable", &enableShortCircuit_);
			ImGui::DragFloat("Interval Min", &shortCircuitIntervalMin_, 0.01f, 0.01f, 2.0f, "%.2fs");
			ImGui::DragFloat("Interval Max", &shortCircuitIntervalMax_, 0.01f, 0.01f, 2.0f, "%.2fs");
			ImGui::SliderFloat("Arc Chance", &shortCircuitArcChance_, 0.0f, 1.0f);
			ImGui::DragInt("Spark Count", &shortCircuitSparkCount_, 1.0f, 0, 64);
			ImGui::DragFloat("Spark Speed", &shortCircuitSparkSpeed_, 0.1f, 0.0f, 20.0f);
			ImGui::ColorEdit4("Spark Color", &shortCircuitSparkColor_.x);
			ImGui::ColorEdit4("Arc Color", &shortCircuitArcColor_.x);
			ImGui::SliderFloat("Body Brightness", &shortCircuitBodyBrightness_, 0.0f, 1.0f);
			ImGui::DragFloat("Body Fade Speed", &shortCircuitFadeSpeed_, 0.1f, 0.1f, 30.0f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Anchor Fray & Hold Retract")) {
			ImGui::Text("Hold Timer: %.2f / %.2f", anchorHoldTimer_, kAnchorHoldTime);
			ImGui::Text("Fray Active: %s (Timer: %.2f / %.2f)", isFrayActive_ ? "true" : "false", frayTimer_, kFrayDuration);
			ImGui::ColorEdit4("Line Color", &lineColor_.x);
			ImGui::TreePop();
		}

		// テレポート演出・GPU雷霆エフェクトの総合調整UI
		if (ImGui::TreeNode("Teleport & Effect Settings")) {
			// テスト用トリガーボタン
			if (ImGui::Button("Trigger Test Teleport Effect")) {
				Vector3 startPos = worldTransform_.translate;
				Vector3 testTargetPos = { startPos.x + 3.0f, startPos.y + 1.5f, startPos.z };
				EmitTeleportLightning(startPos, testTargetPos);
				if (enableTeleportBlur_) {
					teleportBlurTimer_ = teleportBlurMaxTime_;
				}
				if (enableTeleportShake_ && camera_) {
					camera_->StartShake(teleportShakeIntensity_, teleportShakeDuration_);
				}
			}

			ImGui::DragFloat("Teleport Interval", &teleportInterval_, 0.01f, 0.01f, 1.0f, "%.2fs");
			ImGui::DragFloat("Post-Teleport Float", &postTeleportFloatDuration_, 0.01f, 0.0f, 1.0f, "%.2fs");

			if (ImGui::TreeNode("1. GPU Lightning (雷霆エフェクト)")) {
				ImGui::Checkbox("Enable Lightning", &enableTeleportLightning_);
				ImGui::ColorEdit4("Core Color (Center)", &lightningCoreColor_.x);
				ImGui::ColorEdit4("Glow Color (Outer)", &lightningColor_.x);
				ImGui::DragFloat("Duration (s)", &lightningShowDuration_, 0.01f, 0.01f, 1.0f, "%.2fs");
				ImGui::DragFloat("Intensity (Bloom)", &lightningIntensity_, 0.1f, 0.5f, 15.0f, "%.1f");
				ImGui::DragFloat("Bolt Scale (Thickness)", &lightningBoltScale_, 0.01f, 0.01f, 1.0f, "%.2f");
				ImGui::DragFloat("Bolt Radius (Zigzag)", &lightningBoltRadius_, 0.01f, 0.0f, 2.0f, "%.2f");
				ImGui::SliderInt("Subdivisions", &lightningSubdivisions_, 2, 9);
				ImGui::SliderInt("Main Branches", &lightningMainBranches_, 0, 10);
				ImGui::SliderInt("Sparks Per Segment", &lightningSparksPerSegment_, 0, 10);
				ImGui::SliderInt("Impact Sparks", &lightningImpactSparks_, 0, 300);
				ImGui::SliderInt("Start Sparks", &lightningStartSparks_, 0, 150);
				ImGui::DragFloat("Spark Speed", &lightningSparkSpeed_, 0.2f, 1.0f, 40.0f, "%.1f");
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("2. Radial Blur (ラジアルブラー)")) {
				ImGui::Checkbox("Enable Blur", &enableTeleportBlur_);
				ImGui::DragFloat("Blur Duration (s)", &teleportBlurMaxTime_, 0.01f, 0.01f, 1.0f, "%.2fs");
				ImGui::DragFloat("Max Blur Width", &teleportBlurWidth_, 0.001f, 0.001f, 0.1f, "%.4f");
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("3. Camera Shake (カメラシェイク)")) {
				ImGui::Checkbox("Enable Camera Shake", &enableTeleportShake_);
				ImGui::DragFloat("Anchor Shake Intensity", &teleportShakeIntensity_, 0.05f, 0.0f, 3.0f, "%.2f");
				ImGui::DragFloat("Anchor Shake Duration", &teleportShakeDuration_, 0.01f, 0.01f, 1.0f, "%.2fs");
				ImGui::DragFloat("Chain Lock-on Shake Intensity", &lockonTeleportShakeIntensity_, 0.05f, 0.0f, 3.0f, "%.2f");
				ImGui::DragFloat("Chain Lock-on Shake Duration", &lockonTeleportShakeDuration_, 0.01f, 0.01f, 1.0f, "%.2fs");
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("4. Hit Stop (ヒットストップ)")) {
				ImGui::Checkbox("Enable Hit Stop", &enableHitStop_);
				ImGui::DragFloat("Mid-Chain Duration", &teleportMidHitStopDuration_, 0.01f, 0.0f, 1.0f, "%.2fs");
				ImGui::DragFloat("Finish Duration", &teleportFinishHitStopDuration_, 0.01f, 0.0f, 1.0f, "%.2fs");
				ImGui::TreePop();
			}

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

void Player::ResetDamageEffect() {
	damageEffectTimer_ = 0.0f;
	auto sceneManager = Bonjin::SceneManager::GetInstance();
	sceneManager->SetFullScreenVignette(false);
	sceneManager->SetFullScreenVignetteColor(origVignetteColor_);
	sceneManager->SetFullScreenVignetteScale(origVignetteScale_);
	sceneManager->SetFullScreenVignettePower(origVignettePower_);
}


void Player::UpdateStockGainEffect() {
	const int gained = anchorStock_ - prevAnchorStockForFx_;
	prevAnchorStockForFx_ = anchorStock_;
	if (!enableStockGainRing_ || gained <= 0) {
		return;
	}

	GpuParticleManager* gpu = GpuParticleManager::GetInstance();
	const Vector3 center = { worldTransform_.translate.x, worldTransform_.translate.y, worldTransform_.translate.z - 0.3f };

	// 満タンになった時は少し大きく・白寄りに
	const bool isFull = anchorStock_ >= kMaxAnchorStock;
	Vector4 color = stockGainRingColor_;
	float scale = stockGainRingScale_;
	if (isFull) {
		color = { color.x + (1.0f - color.x) * 0.3f, color.y + (1.0f - color.y) * 0.3f, color.z + (1.0f - color.z) * 0.3f, color.w };
		scale *= 1.25f;
	}

	// 一度に複数回復した場合は、サイズ違いの輪を重ねて段数を表現
	for (int i = 0; i < gained; ++i) {
		const float s = scale * (1.0f + 0.35f * static_cast<float>(i));
		gpu->EmitGlossRing(center, color, s, stockGainRingLifeTime_ + 0.08f * static_cast<float>(i), stockGainRingIntensity_, stockGainRingSpin_);
	}
	// 中心の小さなきらめき
	gpu->EmitFlashGlow(center, color, scale * 0.35f, 0.15f, stockGainRingIntensity_);
}

void Player::UpdateShortCircuitEffect() {
	const float dt = 1.0f / 60.0f;
	const bool isShorting = enableShortCircuit_ && anchorStock_ <= 0;

	// 本体の明るさ（ショート中はずっと暗いまま）
	const float targetBrightness = isShorting ? shortCircuitBodyBrightness_ : 1.0f;
	const float t = (std::min)(1.0f, shortCircuitFadeSpeed_ * dt);
	currentBodyBrightness_ += (targetBrightness - currentBodyBrightness_) * t;
	const float brightness = currentBodyBrightness_;
	if (model_) {
		model_->SetColor({ brightness, brightness, brightness, 1.0f });
	}

	if (!isShorting) {
		shortCircuitTimer_ = 0.0f; // 0になった瞬間に即バチッと出るようにリセット
		return;
	}

	shortCircuitTimer_ -= dt;
	if (shortCircuitTimer_ > 0.0f) {
		return;
	}

	static std::mt19937 randomEngine(std::random_device{}());
	std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
	std::uniform_real_distribution<float> distSigned(-1.0f, 1.0f);

	const float intervalMin = (std::min)(shortCircuitIntervalMin_, shortCircuitIntervalMax_);
	const float intervalMax = (std::max)(shortCircuitIntervalMin_, shortCircuitIntervalMax_);
	shortCircuitTimer_ = intervalMin + (intervalMax - intervalMin) * dist01(randomEngine);

	GpuParticleManager* gpu = GpuParticleManager::GetInstance();
	const Vector3 center = worldTransform_.translate;

	// 体の表面付近のランダムな一点から火花が噴き出す
	Vector3 sparkPos = {
		center.x + distSigned(randomEngine) * kWidth * 0.45f,
		center.y + distSigned(randomEngine) * kHeight * 0.45f,
		center.z - 0.3f,
	};
	const int sparkCount = shortCircuitSparkCount_ + static_cast<int>(dist01(randomEngine) * shortCircuitSparkCount_);
	gpu->EmitSparks(sparkPos, sparkCount, shortCircuitSparkColor_, shortCircuitSparkSpeed_, 0.35f, 0.07f);

	// 一瞬のフラッシュ
	gpu->EmitFlashGlow(sparkPos, shortCircuitArcColor_, 0.6f + 0.5f * dist01(randomEngine), 0.08f, 2.5f);

	// 時々、体の上を走る小さな放電アーク
	if (dist01(randomEngine) < shortCircuitArcChance_) {
		const float angle = dist01(randomEngine) * 2.0f * std::numbers::pi_v<float>;
		const float length = 0.6f + 0.6f * dist01(randomEngine);
		GpuLightningConfig arc{};
		arc.start = sparkPos;
		arc.end = {
			sparkPos.x + std::cos(angle) * length,
			sparkPos.y + std::sin(angle) * length,
			sparkPos.z,
		};
		arc.coreColor = { 1.0f, 0.9f, 0.8f, 1.0f };
		arc.glowColor = shortCircuitArcColor_;
		arc.boltRadius = 0.18f;
		arc.boltScale = 0.08f;
		arc.subdivisions = 4;
		arc.mainBranches = 1;
		arc.sparksPerSegment = 0;
		arc.impactSparks = 6;
		arc.duration = 0.07f + 0.05f * dist01(randomEngine);
		arc.intensity = 3.0f;
		arc.sparkSpeed = 3.0f;
		gpu->EmitLightning(arc);
	}
}

void Player::EmitTeleportLightning(const Vector3& start, const Vector3& end) {
	if (!enableTeleportLightning_) {
		return;
	}

	GpuLightningConfig config{};
	config.start = start;
	config.end = end;
	config.coreColor = lightningCoreColor_;
	config.glowColor = lightningColor_;
	config.boltRadius = lightningBoltRadius_;
	config.boltScale = lightningBoltScale_;
	config.subdivisions = lightningSubdivisions_;
	config.mainBranches = lightningMainBranches_;
	config.sparksPerSegment = lightningSparksPerSegment_;
	config.impactSparks = lightningImpactSparks_;
	config.duration = lightningShowDuration_;
	config.intensity = lightningIntensity_;
	config.sparkSpeed = lightningSparkSpeed_;

	GpuParticleManager::GetInstance()->EmitLightning(config);

	// テレポート開始位置にも残留スパークを発生させて瞬間移動感を強化
	if (lightningStartSparks_ > 0) {
		GpuParticleManager::GetInstance()->EmitSparks(
			start,
			lightningStartSparks_,
			lightningColor_,
			lightningSparkSpeed_ * 0.8f,
			lightningShowDuration_ * 1.5f,
			lightningBoltScale_ * 0.5f
		);
	}
}

