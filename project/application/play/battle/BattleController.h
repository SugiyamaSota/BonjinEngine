#pragma once

#include <list>
#include <memory>
#include <random>
#include <vector>

#include "gameObject/enemy/BaseEnemy.h"
#include "gameObject/player/Player.h"
#include "mapchip/MapChipField.h"
#include "Object3D.h"
#include "ParticleManager.h"
#include "SkyBox.h"
#include "gameObject/GameObject.h"
#include "gameObject/goal/Goal.h"
#include "gameObject/camera/CameraController.h"
#include "hud/HUD.h"
#include "hud/GoSignalBanner.h"
#include "hud/AnchorStockIndicator.h"

namespace Bonjin {

/// <summary>
/// GameSceneとTutorialSceneがそれぞれ所有するバトル進行処理
/// </summary>
class BattleController {
public:
	void Initialize(Camera* camera, const char* mapFilePath);
	void Unload();
	void Update(float deltaTime);
	void Draw();
	void DrawImGui();

	Player* GetPlayer() const { return player_.get(); }
	MapChipField* GetMapChipField() const { return mapChipField_.get(); }
	HUD* GetHUD() const { return hud_.get(); }
	const std::list<std::unique_ptr<BaseEnemy>>& GetEnemies() const { return enemies_; }
	void SetEnemyRespawnEnabled(bool enabled) { isEnemyRespawnEnabled_ = enabled; }
	bool IsEnemyRespawnEnabled() const { return isEnemyRespawnEnabled_; }
	bool IsGoalReached() const { return isGoalReached_; }
	bool IsHitStopActive() const { return hitStopTimer_ > 0.0f; }
	// ゴールの位置 (ゴールが無ければ原点)
	Vector3 GetGoalPosition() const { return goal_ ? goal_->GetPosition() : Vector3{}; }

	CameraController* GetCameraController() const { return cameraController_.get(); }
	Camera* GetCamera() const { return camera_; }
	void UpdateVisuals(float deltaTime);

	/// <summary>
	/// クリア演出を開始する (HUDを隠してプレイヤーのFlairを始める)
	/// </summary>
	void StartClearPerformance();

	/// <summary>
	/// クリア演出の更新。プレイヤーの演出が終わったらtrue
	/// </summary>
	bool UpdateClearPerformance(float deltaTime);
	void SetHudVisible(bool visible) { isHudVisible_ = visible; }
	void TriggerGoSignal();
	bool IsGoSignalActive() const { return goSignalBanner_ && goSignalBanner_->IsActive(); }

private:
	bool isHudVisible_ = true;
	Camera* camera_ = nullptr;
	std::unique_ptr<CameraController> cameraController_;
	std::unique_ptr<GoSignalBanner> goSignalBanner_;

	std::unique_ptr<Player> player_;
	std::unique_ptr<Object3D> playerModel_;

	std::list<std::unique_ptr<BaseEnemy>> enemies_;
	std::list<std::unique_ptr<Object3D>> enemyModels_;
	std::list<BaseEnemy*> lockedOnEnemies_;

	std::unique_ptr<MapChipField> mapChipField_;
	std::vector<std::vector<std::unique_ptr<Object3D>>> blockModels_;
	std::vector<std::vector<WorldTransform>> blockWorldTransforms_;
	std::unique_ptr<Goal> goal_;
	// ゴールの更新 (クリア演出の状態をポータル演出に渡してから更新する)
	void UpdateGoal(float deltaTime);
	// クリア演出中、カメラが寄るたびにポータルを雷の色へ近づける割合 (0:ふだんの色 1:雷の色)
	float GetGoalLightningBlend() const;
	// Flairループ中にカメラが寄る回数の合計
	int GetClearPulseTotal() const;
	std::unique_ptr<SkyBox> skyBox_;

	ParticleManager* particleManager_ = nullptr;
	std::mt19937 randomEngine_{std::random_device{}()};

	std::unique_ptr<HUD> hud_;
	std::unique_ptr<AnchorStockIndicator> anchorStockIndicator_;

	bool isEnemyRespawnEnabled_ = false; // チュートリアルのみ有効化する
	float enemyRespawnTime_ = 3.0f;
	bool isGoalReached_ = false;
	float hitStopTimer_ = 0.0f;

	// --- クリア演出のカメラ ---
	// Flair開始の間に通常距離からこの距離まで寄る
	float clearCameraCloseRadius_ = 22.0f;
	// Flairループ中、1回転ごとに一瞬この距離まで寄って戻る
	float clearCameraPulseRadius_ = 15.0f;
	// 寄る時間 / 戻る時間 (秒)
	float clearCameraPulseInTime_ = 0.06f;
	float clearCameraPulseOutTime_ = 0.35f;
	// 1回転の何秒目で寄るか (回転の勢いに合わせる用)
	float clearCameraPulseOffset_ = 0.0f;
	// 1回転あたり何回寄るか (2なら半回転ごと、0.5なら2回転に1回)
	float clearCameraPulsesPerRotation_ = 2.0f;

	bool isClearCameraActive_ = false;

	// カメラが寄るたびに、プレイヤーモデルを中心に敵撃破のパーティクルをランダムな角度に傾けて重ねて出す
	int clearBurstCount_ = 4;            // 1回に出す数
	float clearBurstMaxTiltDeg_ = 65.0f; // 傾ける最大角度(度)。90に近いと真横からになりリングが見えなくなる
	int lastClearPulseIndex_ = -1;       // 何回目の寄りまで出したか

	// 今が何回目の寄りか (Flairループ中以外は-1)
	int GetClearPulseIndex() const;
	// 新しい寄りが始まったらパーティクルを出す
	void UpdateClearPulseEffects();
	void EmitClearBurst();

	// --- パーティクル確認用 (ImGui) ---
	enum class PreviewEffect {
		kAnchorHit,       // アンカー命中 (anchorHitRay + anchorHitFlash)
		kEnemyDefeat,     // 敵撃破 (enemyDefeatSpark + enemyDefeatRing)
		kLandingDust,     // 着地の砂ぼこり (landingDust)
		kEnemyChargeRing, // 敵のチャージ (enemyChargeRing)
		kCount,
	};
	int previewEffect_ = 0;
	bool previewFollowPlayer_ = true;        // プレイヤー位置を基準にするか
	Vector3 previewOffset_ = { 4.0f, 0.0f, 0.0f }; // 基準位置からのずらし
	Vector3 previewPosition_ = {};           // 基準位置を固定するときの座標
	bool previewAutoEmit_ = false;           // 一定間隔で出し続ける
	float previewInterval_ = 0.5f;
	float previewTimer_ = 0.0f;

	Vector3 GetPreviewPosition() const;
	void EmitPreviewEffect(PreviewEffect effect);
	void UpdateParticlePreview(float deltaTime);
	void DrawParticlePreviewImGui();

	// クリア演出中のカメラ距離を計算する
	float CalculateClearCameraRadius() const;
	// Flairループ中のカメラの寄り具合 (0:ふだん 1:一番寄った瞬間)。ループ中以外は0
	float GetClearCameraPulse() const;

	void GenerateBlocksAndGoal();
	void ResolveAnchorInstantLanding(class Anchor& anchor);
	void EmitAnchorHitEffect(const Vector3& position);
	// tilt: エフェクト全体の傾き(ラジアン, XYZ)。リングの面と火花の飛ぶ面が一緒に傾く
	void EmitEnemyDefeatEffect(const Vector3& position, const Vector3& tilt = { 0.0f, 0.0f, 0.0f });
	void EmitLandingDustEffect(const Vector3& position);
};

}
