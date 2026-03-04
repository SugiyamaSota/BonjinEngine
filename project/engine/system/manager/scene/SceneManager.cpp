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

void SceneManager::DestroyInstance() {
	if (instance == nullptr) return;

	for (auto& pair : instance->scenes_) {
		if (pair.second) {
			pair.second->Unload();
		}
	}

	instance->scenes_.clear();

	instance->camera_.reset();

	delete instance;
	instance = nullptr;
}

void SceneManager::Initialize() {
	camera_ = std::make_unique<Camera>();
	camera_->Initialize(1280, 720);
}

// 💡 4. シーンの登録
void SceneManager::AddScene(SceneType type, std::unique_ptr<IScene> scene) {
	// 既に登録されているかチェック（オプション）
	if (scenes_.find(type) != scenes_.end()) {
		// エラー処理または上書き処理
		return;
	}

	scenes_[type] = std::move(scene);
	// 最初のシーンであれば、カレントシーンとして設定
	if (currentScene_ == nullptr) {
		currentScene_ = scenes_[type].get();
		currentScene_->Initialize(camera_.get());
	}
}

// 💡 5. シーンの更新処理
void SceneManager::Update(float deltaTime) {
	if (currentScene_ == nullptr) {
		return;
	}

	camera_->Update(Camera::CameraType::kDebug);

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

void SceneManager::ChangeScene(SceneType nextSceneType) {
	if (nextSceneType == SceneType::kExit) {
		// 終了処理
		return;
	}

	if (currentScene_ != nullptr) {
		currentScene_->Unload();
	}

	auto it = scenes_.find(nextSceneType);
	if (it == scenes_.end()) return;

	// unique_ptr が管理する実体のアドレスをセット
	currentScene_ = it->second.get();
	currentScene_->Initialize(camera_.get());
}