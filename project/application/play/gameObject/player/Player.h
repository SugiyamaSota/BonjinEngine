#pragma once
#include <cassert>
#include <list>
#include <memory>
#include <numbers>

#include "../anchor/anchor.h"
#include "../../logic/Data.h"
#include "../GameObject.h"

#include "Object3D.h"
#include "Line3D.h"
#include "Lightning3D.h"
#include "FrayLine3D.h"

#include "PlayerStatusComponent.h"
#include "PlayerAnimator.h"
#include "PlayerClearPerformance.h"

class MapChipField;
namespace Bonjin {
	class BaseEnemy;
}
class Camera;

/// <summary>
/// 自キャラ
/// </summary>
class Player : public Bonjin::GameObject {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Object3D* model, Camera* camera, const Vector3& position);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();


	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// アンカーラインの描画処理（描画順序を最後に調整するため個別化）
	/// </summary>
	void DrawAnchorLine();

	/// <summary>
	/// 移動処理
	/// </summary>
	void Move();

protected:
	void OnMapCollision(const CollisionMapInfo& collisionMapinfo) override;

public:
	// モデル表示用の補正を加えて描画行列を更新
	void UpdateModel() override;

	// 描画モデルのワールド変換 (足元合わせ・向き補正込み)
	WorldTransform GetModelWorldTransform() const;
	// 演出用の倍率を掛ける前のモデルのワールド変換
	WorldTransform GetModelBaseWorldTransform() const;
	// 演出用の見た目の大きさの倍率 (当たり判定には影響しない。1.0で通常)
	void SetVisualScale(float scale) { visualScaleMultiplier_ = scale; }
	float GetVisualScale() const { return visualScaleMultiplier_; }
	// 描画モデルの中心(腰のボーン)のワールド座標。スケルトンが無ければ当たり判定の中心
	Vector3 GetModelCenterPosition() const;

	/// <summary>
	/// 床や天井にめり込んでいたら上下に押し出す (出現・テレポート直後用)
	/// </summary>
	void ResolveMapEmbedding();

	Vector3 GetWorldPosition();
	AABB GetAABB();
	void OnCollision(Bonjin::BaseEnemy* enemy);
	bool ConsumeLandingEffectRequest();
	bool HasAnchor() const {
		return anchor_ != nullptr;
	}

	Anchor& GetAnchor() {
		assert(anchor_); // nullではないことを確認
		return *anchor_;
	}

	/// <summary>
	/// アンカーのゲッター (const版)
	/// </summary>
	const Anchor& GetAnchor() const {
		assert(anchor_); // nullではないことを確認
		return *anchor_;
	}

	void SetLockedOnEnemiesList(std::list<Bonjin::BaseEnemy*>* enemiesList) {
		lockedOnEnemies_ = enemiesList;
	}

	void AddLockedOnEnemy(Bonjin::BaseEnemy* enemy) {
		if (lockedOnEnemies_ && enemy) {
			lockedOnEnemies_->push_back(enemy);
		}
	}

	size_t GetLockedOnEnemiesCount() const {
		return lockedOnEnemies_ ? lockedOnEnemies_->size() : 0;
	}

	void RequestHitStop(float duration) {
		if (!enableHitStop_) return;
		hitStopRequested_ = true;
		hitStopDuration_ = duration;
	}

	bool ConsumeHitStopRequest(float& outDuration) {
		if (!hitStopRequested_) return false;
		hitStopRequested_ = false;
		outDuration = hitStopDuration_;
		return true;
	}

	bool ConsumeHitStopRequest() {
		if (!hitStopRequested_) return false;
		hitStopRequested_ = false;
		return true;
	}

	// HP関連
	void ApplyDamage(int damage);
	/// <summary>
	/// ダメージVignette演出を即時キャンセルする（deathPhase移行時など）
	/// </summary>
	void ResetDamageEffect();

	// レベル・経験値関連
	void GainExp(int amount);

	bool GetIsInvincible() const { return isInvincible_; }

	bool GetIsGoalReached() const { return isGoalReached_; }
	void SetIsGoalReached(bool reached) { isGoalReached_ = reached; }

	/// <summary>
	/// クリア演出(Flair)を開始する。以降はUpdateではなくUpdateClearPerformanceを呼ぶ
	/// </summary>
	void StartClearPerformance();

	/// <summary>
	/// クリア演出の更新 (入力は受け付けず、着地を待ってからFlairを再生する)
	/// </summary>
	void UpdateClearPerformance();

	/// <summary>
	/// クリア演出のFlairが最後まで終わったか
	/// </summary>
	bool IsClearPerformanceFinished() const { return clearPerformance_.IsFinished(); }

	// クリア演出の段階
	using ClearPhase = PlayerClearPhase;
	ClearPhase GetClearPhase() const { return clearPerformance_.GetPhase(); }
	// 今の段階に入ってからの経過時間(秒)
	float GetClearPhaseTimer() const { return clearPerformance_.GetTimer(); }
	// クリア演出のカメラの寄り具合 (0:ふだんの距離 1:寄りの距離)。段階ごとの違いは各段階のクラスが決める
	float GetClearCameraZoom() const { return clearPerformance_.GetCameraZoom(); }
	// ゴールのポータルを雷の色へ近づける割合 (loopProgress: Flairループで寄った回数の進み具合 0~1)
	float GetClearLightningBlend(float loopProgress) const { return clearPerformance_.GetLightningBlend(loopProgress); }
	// アニメーションの再生長さ(秒)。カメラ演出の同期用
	float GetAnimationPlayLength(PlayerAnimator::State state) const { return hasAnimator_ ? animator_.GetClipPlayLength(state) : 0.0f; }
	// Flairループの回数 (カメラの寄りの回数を数える用)
	int GetClearFlairLoopCount() const { return clearPerformance_.GetSettings().flairLoopCount; }
	// テレポートの雷の色 (クリア演出で色を合わせる用)
	const Vector4& GetLightningColor() const { return lightningColor_; }

	// ロックオン中の敵をすべて削除する処理（入力判定を含む)
	void HandleLockOnRemovalInput();

	// ロックオン中の敵をすべて倒す
	void RemoveLockedOnEnemies(std::list<Bonjin::BaseEnemy*>& enemies);

	void DrawImGui();

	// 衝突コールバックのオーバーライド
	void OnCollision(Bonjin::Collider* other) override;

	void EmitAnchorHitEffect(const Vector3& position);

	void UpdateWorldTransform() { model_->Update(worldTransform_, camera_); }

	// ステータス関連
	const PlayerStatusComponent& GetStatus() const { return statusComponent_; }
	PlayerStatusComponent& GetStatus() { return statusComponent_; }

	int GetHp() const { return statusComponent_.GetHp(); }
	int GetMaxHp() const { return statusComponent_.GetMaxHp(); }
	int GetAttackPower() const { return statusComponent_.GetAttackPower(); }
	int GetLevel() const { return statusComponent_.GetLevel(); }
	int GetExp() const { return statusComponent_.GetExp(); }
	int GetRequiredExp() const { return statusComponent_.GetRequiredExp(); }
	float GetAnchorRechargeDuration() const { return statusComponent_.GetAnchorRechargeTime(); }

	// アンカーストック・リチャージ関連
	int GetAnchorStock() const { return anchorStock_; }
	int GetMaxAnchorStock() const { return kMaxAnchorStock; }
	float GetAnchorRechargeTimer() const { return anchorRechargeTimer_; }
	void AddAnchorRechargeProgress(float seconds);
	void RestoreAnchorStock(int count = 1);

	/// <summary>
	/// アンカー入力時のストック表示要求を取得して消費する
	/// </summary>
	/// <param name="outDenied">ストック不足で発射できなかった場合 true</param>
	bool ConsumeAnchorIndicatorRequest(bool& outDenied) {
		if (!anchorIndicatorRequested_) return false;
		anchorIndicatorRequested_ = false;
		outDenied = anchorShootDenied_;
		anchorShootDenied_ = false;
		return true;
	}

private:

	static inline const float kAcceleration = 0.010f;
	static inline const float kAttenuation = 0.8f;
	static inline const float kLimitRunSpeed = 0.15f;

	static inline const float kAccelerationInAir = 0.010f;

	//旋回関連
	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;
	static inline const float kTimeTurn = 0.3f;

	//ジャンプ初速
	static inline const float kJumpAcceleration = 0.32f;

	// --- 当たり判定 ---
	// 衝突後処理の数値
	static inline const float kAttenuationTop = 0.5f;
	static inline const float kAttenuationWall = 0.5f;

	static inline const float kKnockbackUpPower = 0.15f;
	static inline const float kKnockbackPower = 0.15f;

	bool landingEffectRequested_ = false;

	bool isKnockedBack_ = false;
	float knockbackTimer_ = 0.0f;
	static inline const float kKnockbackTime = 1.0f;
	static inline const float kKnockbackAttenuation = 0.95f;

	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;
	static inline const float kInvincibleTime = 1.5f;

	// 当たり判定 (見た目のロボットに合わせる)
	// 高さ: モデルの身長(約1.0) * 見た目の倍率 → 約2マス
	// 幅  : 横から見た体の厚み(約0.19*3.8≒0.72)に、走りで手足が前後に振れる分を足したもの
	static inline const float kHeight = 3.8f;
	static inline const float kWidth = 1.0f;

	// プレイヤーモデルの見た目補正 (モデルは足元原点・高さ約1.0)
	// モデルの身長が約1.0なので、当たり判定の高さと同じ倍率にすると身長が当たり判定と一致する
	float modelScale_ = kHeight;
	// 足元を当たり判定の底に合わせる
	float modelOffsetY_ = -kHeight / 2.0f;
	// モデルは+Z(奥)向きなので、回転0で右(+X)を向くよう90度回す
	float modelRotateYOffset_ = std::numbers::pi_v<float> / 2.0f;

	// モデルのアニメーション
	PlayerAnimator animator_;
	bool hasAnimator_ = false;

	// --- クリア演出 (段階ごとの振る舞いは PlayerClearPerformance の各段階のクラスが持つ) ---
	PlayerClearPerformance clearPerformance_;

	// 演出用の見た目の大きさの倍率 (当たり判定には影響しない)
	float visualScaleMultiplier_ = 1.0f;
	// この速さ以上で走りアニメーションにする
	float runAnimationSpeedThreshold_ = 0.01f;

	// Padのデッドゾーン
	long kPadDeadZone_ = 750;

	// --- アンカー関連 ---
	float kAnchorSpeed = 0.1f; // アンカーの初速の大きさ
	static inline const long kAnchorDeadZone = 750; // アンカー発射時のスティック入力のデッドゾーン

	// --- アンカー ---
	std::unique_ptr<Anchor> anchor_;
	std::unique_ptr<Bonjin::Line3D> anchorLine_;
	Vector4 lineColor_ = { 0.5f, 0.85f, 1.f, 0.5f };
	void shootAnchor();

	// アンカーストック管理
	static inline const int kMaxAnchorStock = 3;
	int anchorStock_ = kMaxAnchorStock;
	float anchorRechargeTimer_ = 0.0f;

	// アンカーストック表示（頭上インジケーター）要求
	bool anchorIndicatorRequested_ = false;
	bool anchorShootDenied_ = false;

	// ストック回復時の光沢リング演出
	void UpdateStockGainEffect();
	int prevAnchorStockForFx_ = kMaxAnchorStock;
	bool enableStockGainRing_ = true;
	Vector4 stockGainRingColor_ = { 0.0f, 0.1f, 1.0f, 1.0f }; // 頭上インジケーターのグリーンに合わせる
	float stockGainRingScale_ = 7.5f;
	float stockGainRingLifeTime_ = 0.3f;
	float stockGainRingIntensity_ = 10.0f;
	float stockGainRingSpin_ = 20.0f;          // ハイライトが輪に沿って回る速さ

	// ストック0時の「エンジンショート」演出
	void UpdateShortCircuitEffect();
	bool enableShortCircuit_ = true;
	float shortCircuitTimer_ = 0.0f;           // 次のバチッまでの残り時間
	float shortCircuitIntervalMin_ = 0.02f;
	float shortCircuitIntervalMax_ = 0.12f;
	float shortCircuitArcChance_ = 0.5f;       // 小さな放電アークを伴う確率
	int shortCircuitSparkCount_ = 14;
	float shortCircuitBodyBrightness_ = 0.3f;  // ショート中の本体の明るさ (1.0で通常)
	float shortCircuitFadeSpeed_ = 6.0f;       // 暗転/復帰の速さ
	float currentBodyBrightness_ = 1.0f;
	float shortCircuitSparkSpeed_ = 4.5f;
	Vector4 shortCircuitSparkColor_ = { 1.0f, 0.55f, 0.15f, 1.0f };  // オレンジの火花
	Vector4 shortCircuitArcColor_ = { 1.0f, 0.25f, 0.2f, 1.0f };     // 赤い放電

	// アンカー回収用（長押し判定・ほつれ演出）
	float anchorHoldTimer_ = 0.0f;
	static inline const float kAnchorHoldTime = 0.35f;

	std::unique_ptr<Bonjin::FrayLine3D> frayLineEffect_;
	float frayTimer_ = 0.0f;
	static inline const float kFrayDuration = 0.45f;
	Vector3 lastAnchorRetractPos_{};
	bool isFrayActive_ = false;

	// アンカー回収時のフェードアウト表示用モデル
	std::unique_ptr<Object3D> retractAnchorModel_;
	WorldTransform retractAnchorTransform_{};

	// ロックオンされた敵のリストへのポインタ
	std::list<Bonjin::BaseEnemy*>* lockedOnEnemies_ = nullptr;

	PlayerStatusComponent statusComponent_;

	bool isGoalReached_ = false;

	// 順次テレポート用
	std::list<Vector3> teleportQueue_;
	float teleportTimer_ = 0.0f;
	float teleportInterval_ = 0.15f;

	// テレポート直後の滞空（急落下防止）
	void StartPostTeleportFloat();
	float postTeleportFloatTimer_ = 0.0f;
	float postTeleportFloatDuration_ = 0.2f;

	// GPU雷霆テレポートエフェクト用
	void EmitTeleportLightning(const Vector3& start, const Vector3& end);
	Vector3 lastTeleportStartPos_{};
	bool enableTeleportLightning_ = true;
	Vector4 lightningCoreColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector4 lightningColor_ = { 0.3f, 0.7f, 1.0f, 1.0f };
	float lightningShowDuration_ = 1.0f;
	float lightningBoltRadius_ = 0.35f;
	float lightningBoltScale_ = 0.22f;
	int lightningSubdivisions_ = 7;
	int lightningMainBranches_ = 3;
	int lightningSparksPerSegment_ = 2;
	int lightningImpactSparks_ = 80;
	int lightningStartSparks_ = 25;
	float lightningIntensity_ = 3.5f;
	float lightningSparkSpeed_ = 7.0f;

	// ヒットストップ演出用
	bool hitStopRequested_ = false;
	bool enableHitStop_ = true;
	float hitStopDuration_ = 0.05f;
	float teleportMidHitStopDuration_ = 0.05f;    // 連続テレポート途中
	float teleportFinishHitStopDuration_ = 0.3f;  // 連続テレポート終了時（フィニッシュ）

	// ダメージVignette演出用
	float damageEffectTimer_ = 0.0f;
	static inline const float kDamageEffectMaxTime = 0.5f;
	bool wasVignette_ = false;
	Vector3 origVignetteColor_ = { 0.0f, 0.0f, 0.0f };
	float origVignetteScale_ = 16.0f;
	float origVignettePower_ = 0.8f;

	// テレポートラジアルブラー演出用
	bool enableTeleportBlur_ = true;
	float teleportBlurTimer_ = 0.0f;
	float teleportBlurMaxTime_ = 0.2f;
	float teleportBlurWidth_ = 0.025f;

	// テレポートカメラシェイク演出用
	bool enableTeleportShake_ = true;
	float teleportShakeIntensity_ = 0.3f;
	float teleportShakeDuration_ = 0.08f;
	float lockonTeleportShakeIntensity_ = 0.5f;
	float lockonTeleportShakeDuration_ = 0.1f;

};
