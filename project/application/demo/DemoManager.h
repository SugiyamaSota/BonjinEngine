#pragma once
#include "DemoPlayerController.h"

namespace Bonjin {

/// <summary>
/// 勝手にループするデモ (展示用)
/// デモモードの間は「タイトル → 一定時間で自動でゲームへ → プレイヤーが自動でゴールまで進む → クリア演出 → タイトル」をくり返す
/// プレイヤーの自動操作そのものは DemoPlayerController (プレイヤーに命令を出すAI) が行う
/// </summary>
class DemoManager {
public:
	static DemoManager* GetInstance();

	// デモモードのON/OFF (ImGuiで切り替える。起動時の値は kDefaultDemoMode)
	void SetDemoMode(bool enabled);
	bool IsDemoMode() const { return isDemoMode_; }

	// タイトルで何秒待ってからゲームへ進むか
	float GetTitleWaitTime() const { return titleWaitTime_; }

	// 自動操作のパラメータ (DemoPlayerController が参照する)
	const DemoAutoPlaySettings& GetAutoPlaySettings() const { return autoPlaySettings_; }

	// --- ゲーム中 ---
	// ゲーム開始時に呼ぶ
	void BeginGame();
	// ゲーム終了時に呼ぶ
	void EndGame();
	/// <summary>
	/// プレイ中、毎フレーム呼ぶ
	/// ゴールできずに長くかかりすぎたら、あきらめてタイトルへ戻る
	/// </summary>
	void UpdatePlayTimer(float deltaTime);

	void DrawImGui();

private:
	DemoManager() = default;
	DemoManager(const DemoManager&) = delete;
	DemoManager& operator=(const DemoManager&) = delete;

	// 起動したときにデモモードにするか (展示ではtrueにする)
	static inline const bool kDefaultDemoMode = false;

	bool isDemoMode_ = kDefaultDemoMode;
	bool isInGame_ = false;
	float titleWaitTime_ = 5.0f;

	DemoAutoPlaySettings autoPlaySettings_;

	// ゴールできずに詰まったときの保険。この秒数たったらタイトルへ戻る
	float maxPlayTime_ = 60.0f;
	float playTimer_ = 0.0f;
	bool isGiveUpRequested_ = false;
};

}
