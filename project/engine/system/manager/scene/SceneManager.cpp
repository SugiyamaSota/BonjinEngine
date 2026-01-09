#include "SceneManager.h"
#include "time/Time.h" // デルタタイムを取得するため（Timeクラスのパスは適宜修正）

using namespace Bonjin;

// 💡 1. シングルトンインスタンスの実体
SceneManager* SceneManager::instance = nullptr;

// 💡 2. シングルトン: インスタンスの取得
SceneManager* SceneManager::GetInstance() {
	if (instance == nullptr) {
		instance = new SceneManager();
	}
	return instance;
}

// 💡 3. シングルトン: インスタンスの破棄
void SceneManager::DestroyInstance() {
	// 登録されているシーンをすべて破棄（メモリリーク防止）
	for (auto& pair : instance->scenes_) {
		pair.second->Unload();
		delete pair.second;
	}

	instance->scenes_.clear();
	delete instance->camera;

	delete instance;
	instance = nullptr;
}

void SceneManager::Initialize() {
	camera = new Camera();
	camera->Initialize(1280, 720);
}

// 💡 4. シーンの登録
void SceneManager::AddScene(SceneType type, IScene* scene) {
	// 既に登録されているかチェック（オプション）
	if (scenes_.find(type) != scenes_.end()) {
		// エラー処理または上書き処理
		return;
	}

	scenes_[type] = scene;

	// 最初のシーンであれば、カレントシーンとして設定
	if (currentScene_ == nullptr) {
		currentScene_ = scene;
		currentScene_->Initialize(camera);
	}
}

// 💡 5. シーンの更新処理
void SceneManager::Update(float deltaTime) {
	if (currentScene_ == nullptr) {
		return;
	}

	camera->Update(Camera::CameraType::kDebug);

	// 現在のシーンの更新処理を呼び出し
	currentScene_->Update(deltaTime);

	// 遷移先のシーンタイプを取得
	SceneType nextType = currentScene_->GetNextScene();

	// 現在のシーンと次のシーンが異なる場合、シーンを切り替える
	if (nextType != currentScene_->GetCurrentSceneType()) {
		ChangeScene(nextType);
	}
}

// 💡 6. シーンの描画処理
void SceneManager::Draw() {
	if (currentScene_ == nullptr) {
		return;
	}

	currentScene_->Draw();

	currentScene_->DrawImGui();
}

// 💡 7. シーン切り替えロジック（プライベート関数）
void SceneManager::ChangeScene(SceneType nextSceneType) {

	// 終了シーンならゲームを終了
	if (nextSceneType == SceneType::kExit) {
		// WinAppの終了フラグを立てるなどの処理
		return;
	}

	if (currentScene_ != nullptr) {
		currentScene_->Unload();
	}

	// 遷移先のシーンが登録されているか確認
	auto it = scenes_.find(nextSceneType);
	if (it == scenes_.end()) {
		// エラー: 遷移先が登録されていません
		return;
	}

	// 💡 シーンの切り替え実行
	IScene* nextScene = it->second;

	// 1. 新しいシーンを初期化
	nextScene->Initialize(camera);

	// 2. カレントシーンを更新
	currentScene_ = nextScene;

	// 3. （オプション）ここでフェードアウトの完了とフェードインの開始処理を行う
}