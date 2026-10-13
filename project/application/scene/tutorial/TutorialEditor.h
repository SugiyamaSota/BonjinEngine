#pragma once
#include <string>
#include <filesystem>
#include <memory>

namespace Bonjin {

class TutorialManager;

/// <summary>
/// チュートリアルのパラメータ編集・JSON管理・ホットリロードを行うエディタクラス
/// </summary>
class TutorialEditor {
public:
	TutorialEditor();
	~TutorialEditor();

	/// <summary>
	/// 初期化（対象のTutorialManagerを紐付け）
	/// </summary>
	/// <param name="tutorialManager">編集対象のTutorialManager</param>
	void Initialize(TutorialManager* tutorialManager);

	/// <summary>
	/// 更新処理（ファイルのホットリロード監視など）
	/// </summary>
	/// <param name="deltaTime">フレーム時間</param>
	void Update(float deltaTime);

	/// <summary>
	/// ImGuiによるエディタUI描画
	/// </summary>
	void DrawImGui();

	/// <summary>
	/// 現在の設定・ステップをJSONファイルに保存
	/// </summary>
	bool Save();

	/// <summary>
	/// JSONファイルから設定・ステップを再読み込み
	/// </summary>
	bool Reload();

	/// <summary>
	/// JSONファイルパスの設定・取得
	/// </summary>
	void SetJsonFilePath(const std::string& path) { jsonFilePath_ = path; }
	const std::string& GetJsonFilePath() const { return jsonFilePath_; }

	/// <summary>
	/// ホットリロードの有効/無効
	/// </summary>
	void SetAutoReloadEnabled(bool enable) { autoReloadEnabled_ = enable; }
	bool IsAutoReloadEnabled() const { return autoReloadEnabled_; }

private:
	TutorialManager* tutorialManager_ = nullptr;
	std::string jsonFilePath_ = "resources/data/tutorial.json";
	std::filesystem::file_time_type lastFileWriteTime_{};
	bool autoReloadEnabled_ = true;
	float fileCheckTimer_ = 0.0f;

	std::string statusMessage_;
	float statusMessageTimer_ = 0.0f;

	void CheckHotReload();
	void SetStatus(const std::string& msg, float duration = 3.0f);
};

}
