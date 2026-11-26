#pragma once
#include "../GameObject.h"
#include "Object3D.h"
#include <memory>

class Camera;

namespace Bonjin {

/// <summary>
/// ゴール (転送パッド + ポータル演出)
/// 当たり判定はマスの中心、見た目は床の上に置く
/// </summary>
class Goal : public GameObject {
public:
	// ポータルのパーティクルグループ名
	static inline const char* const kPortalPillarGroup = "goalPortalPillar";
	static inline const char* const kPortalRingGroup = "goalPortalRing";

	/// <summary>
	/// ポータルのパーティクルグループを作る (起動時に1回だけ呼ぶ)
	/// </summary>
	static void CreatePortalParticleGroups();

	/// <summary>
	/// 破棄するとき、自分を参照しているポータルのパーティクルも消す
	/// </summary>
	~Goal() override;

	/// <summary>
	/// 初期化 (positionはマップチップの中心)
	/// </summary>
	void Initialize(Camera* camera, const Vector3& position);

	/// <summary>
	/// 更新 (当たり判定・見た目・ポータル演出)
	/// </summary>
	void Update(float deltaTime);

	/// <summary>
	/// ImGui描画 (ポータル演出の調整)
	/// </summary>
	void DrawImGui();

	/// <summary>
	/// 光の柱を出す (寿命を長くして出しっぱなしにする)
	/// </summary>
	void EmitPortalPillar();

	/// <summary>
	/// ポータルのパーティクルを全て消す (シーンをまたいで残さない用)
	/// </summary>
	static void ClearPortalParticles();

	/// <summary>
	/// クリア演出でカメラが寄った具合 (0:ふだん 1:一番寄った瞬間)。柱が太く・高くなる
	/// </summary>
	void SetClearPulse(float pulse) { clearPulse_ = pulse; }

	/// <summary>
	/// ポータルの目標の色を ふだんの色 -> lightningColor へ blend (0~1) だけ近づける
	/// </summary>
	void SetLightningBlend(const Vector4& lightningColor, float blend) {
		lightningColor_ = lightningColor;
		lightningBlend_ = blend;
	}

private:
	// 引数なしの GameObject::Update() は見た目の位置の上書きとポータル演出をしないので、外から呼べないようにする
	using GameObject::Update;

	// 台座の上面の中心
	Vector3 GetPortalBase() const;
	void EmitPortalRing();
	// リングを一定間隔で出す
	void UpdatePortal(float deltaTime);
	// 目標の色へすっと近づける
	void UpdatePortalColor(float deltaTime);

	std::unique_ptr<Object3D> goalModel_;
	// ゴールの見た目の配置 (床の上に置くため当たり判定とは別に持つ)
	WorldTransform modelTransform_{};
	static inline const float kPadScale = 0.85f;           // 幅約2.6 -> 約2.2 (ゴールの当たり判定と同じくらい)
	static inline const float kPadHalfHeight = 0.263f;     // goalPad.obj の厚さの半分
	static inline const float kMapBlockHalfHeight = 1.0f;  // マップの1マスの高さ(2.0)の半分
	static inline const float kColliderSize = 2.2f;

	// --- ポータル演出 (台座の上の光の柱 + 下から上へ昇るリング) ---
	Vector4 portalBaseColor_ = { 0.25f, 1.0f, 0.55f, 1.0f }; // ふだんの色 (緑)
	Vector4 portalColor_ = portalBaseColor_;                // 今の色 (クリア演出で雷の色へ変わっていく)
	float portalColorChangeSpeed_ = 12.0f;                  // 色が目標に追いつく速さ (大きいほど早い)
	float pillarRadius_ = 0.8f;
	float pillarHeight_ = 4.5f;
	// クリア演出でカメラが一番寄った瞬間の柱の大きさ (倍率)
	float pillarPulseRadiusScale_ = 1.6f;
	float pillarPulseHeightScale_ = 1.2f;
	float blinkSpeed_ = 2.0f;     // 点滅の速さ (回/秒)
	float blinkMin_ = 0.35f;      // 点滅で一番暗いときの明るさ
	float ringInterval_ = 0.45f;  // リングを出す間隔 (秒)
	float ringLifeTime_ = 1.6f;   // 1つのリングが昇りきるまでの時間 (秒)
	float ringScale_ = 1.6f;      // リングの大きさ (光る輪の半径は約0.6倍)
	float ringTilt_ = 1.5707963f; // リングをX軸まわりに倒す角度 (ラジアン。90度で床と水平)
	float ringTimer_ = 0.0f;
	float portalTime_ = 0.0f;     // 点滅用の経過時間

	// クリア演出から渡される値
	float clearPulse_ = 0.0f;
	Vector4 lightningColor_ = portalBaseColor_;
	float lightningBlend_ = 0.0f;
};

}
