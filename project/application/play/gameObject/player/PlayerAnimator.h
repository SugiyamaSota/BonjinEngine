#pragma once
#include "Animation.h"
#include "Skeleton.h"

#include <array>
#include <map>
#include <string>
#include <vector>

/// <summary>
/// プレイヤーモデルのアニメーション管理
/// Mixamoのアニメーション(glTFに変換したもの)を、ボーン名が同じプレイヤーモデルに流し込む。
///
/// プレイヤーモデル(Tripo製)とMixamoではボーンのローカル軸の取り方が違うため、
/// ローカル回転をそのまま使わず、ワールド空間でリターゲットする。
///   ・元データで「レストポーズからワールド空間でどれだけ回ったか」(Delta)を求め、
///     プレイヤーの同じボーンのレストポーズに掛ける
///   ・Tポーズ(Mixamo)とAポーズ(プレイヤー)の差は、レスト時のボーンの向きを揃える補正(Align)で吸収する
///   ・体格が違うので移動はHipsのみ、腰の高さの比でスケールして適用する
/// </summary>
class PlayerAnimator {
public:
	enum class State {
		kIdle,
		kRun,
		kJump,
		kFlairStart,  // クリア演出: Flair開始
		kFlairing,    // クリア演出: Flairループ
		kFlairFinish, // クリア演出: Flair終了
		kCount,
	};

	/// <summary>
	/// 初期化。skeletonは描画モデルのもの（レストポーズの記録に使う）
	/// </summary>
	void Initialize(const Skeleton& skeleton);

	/// <summary>
	/// 状態の切り替えと時間経過
	/// </summary>
	void Update(float deltaTime, State state);

	/// <summary>
	/// 現在のポーズをスケルトンに書き込む（何度呼んでも同じ結果）
	/// </summary>
	void Apply(Skeleton& skeleton) const;

	void DrawImGui();

	/// <summary>
	/// クリップの再生範囲の長さ(秒, 再生速度込み)
	/// </summary>
	float GetClipPlayLength(State state) const;

	bool IsClipLoaded(State state) const { return clips_[static_cast<size_t>(state)].isLoaded; }

private:
	// ワールド(スケルトン空間)での姿勢
	struct GlobalPose {
		Quaternion rotate{ 0.0f, 0.0f, 0.0f, 1.0f };
		Vector3 position{};
		float scale = 1.0f;
	};

	// 元データのノード
	struct SourceBone {
		std::string name;
		int32_t parent = -1;
		QuaternionTransform rest{};
	};

	struct Clip {
		Animation animation;
		std::vector<SourceBone> bones;               // 親が先に来る順
		std::map<std::string, int32_t> boneIndex;
		std::vector<GlobalPose> restGlobal;          // 元データのレストポーズ(ワールド)
		std::vector<int32_t> targetToSource;         // プレイヤーのジョイント -> 元データのボーン (-1:対応なし)
		std::vector<Quaternion> align;               // プレイヤーのジョイントごとのレスト向き補正
		float heightRatio = 0.0f;                    // プレイヤーの腰の高さ / 元データの腰の高さ
		bool loop = true;
		bool applyRootTranslation = true; // Hipsの上下移動を使うか
		bool applyHorizontalTranslation = false; // Hipsの前後左右の移動も使うか (In Placeのデータ用)
		float speed = 1.0f;
		// 再生範囲(秒)。endTime<0 ならクリップの最後まで
		float startTime = 0.0f;
		float endTime = -1.0f;
		bool isLoaded = false;
	};

	struct Playback {
		State state = State::kIdle;
		float time = 0.0f;
	};

	static Clip LoadClip(const std::string& directoryPath, const std::string& filename, bool loop, bool applyRootTranslation, bool applyHorizontalTranslation = false);

	// プレイヤーのスケルトンとの対応付け・補正の計算
	void BindClip(Clip& clip) const;

	// 1クリップ分のポーズ(プレイヤーのジョイントごとのローカル)を計算する
	void SamplePose(const Playback& playback, std::vector<QuaternionTransform>& outLocal) const;

	void AdvanceTime(Playback& playback, float deltaTime) const;

	// クリップの再生範囲の終わり
	static float GetEndTime(const Clip& clip);

	std::array<Clip, static_cast<size_t>(State::kCount)> clips_{};

	// プレイヤーのスケルトン情報
	std::vector<std::string> targetNames_;
	std::vector<int32_t> targetParents_;
	std::vector<QuaternionTransform> targetRestLocal_;
	std::vector<GlobalPose> targetRestGlobal_;

	Playback current_{};
	Playback previous_{};
	float blendTimer_ = 0.0f; // クロスフェード残り時間
	float blendDuration_ = 0.15f;

	bool isEnabled_ = true;
	bool useAlign_ = true; // Tポーズ/Aポーズの差を補正するか

	// SamplePose用の作業バッファ
	mutable std::vector<QuaternionTransform> poseBuffer_;
	mutable std::vector<QuaternionTransform> previousPoseBuffer_;
};
