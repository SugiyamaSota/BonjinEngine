#include "SceneManager.h"
#include "time/Time.h" // デルタタイムを取得するため（Timeクラスのパスは適宜修正）
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "ImGuiEditorWindows.h"

using namespace Bonjin;

SceneManager* SceneManager::GetInstance() {
	static SceneManager instance;
	return &instance;
}

void SceneManager::Finalize() {
	// 1. 現在のシーンの終了処理を呼ぶ
	if (currentScene_) {
		currentScene_->Unload();
		currentScene_ = nullptr;
	}
	// 2. unique_ptrのマップをクリアして、全シーンのインスタンスを確実に破棄
	scenes_.clear();

	// 3. カメラも明示的に破棄
	camera_.reset();
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
	if (hasPendingSceneChange_) {
		hasPendingSceneChange_ = false;
		hasPendingSceneRestart_ = false;
		ChangeScene(pendingSceneType_);
	}

	if (currentScene_ == nullptr) {
		return;
	}

	if (Input::GetInstance()->IsTrigger(DIK_F5)) {
		RequestSceneRestart();
	}

	if (hasPendingSceneRestart_) {
		hasPendingSceneRestart_ = false;
		RestartCurrentScene();
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

}

void SceneManager::DrawImGui() {
	if (currentScene_ == nullptr) {
		return;
	}

#ifdef USE_IMGUI
	ImGuiEditorWindows::DrawSystemSettings(this);
	ImGuiEditorWindows::DrawHierarchy(currentScene_);
	ImGuiEditorWindows::DrawGameView(this);
	currentScene_->DrawImGui();
#endif
}

void SceneManager::ChangeScene(SceneType nextSceneType) {
	if (nextSceneType == "Exit") {
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

void SceneManager::RequestSceneChange(SceneType nextSceneType) {
	if (currentScene_ != nullptr && nextSceneType == currentScene_->GetCurrentSceneType()) {
		return;
	}

	pendingSceneType_ = nextSceneType;
	hasPendingSceneChange_ = true;
}

void SceneManager::RestartCurrentScene() {
	if (currentScene_ == nullptr) {
		return;
	}

	currentScene_->Unload();
	currentScene_->Initialize(camera_.get());
}

void SceneManager::RequestSceneRestart() {
	if (currentScene_ == nullptr || hasPendingSceneChange_) {
		return;
	}

	hasPendingSceneRestart_ = true;
}

void SceneManager::SetPostEffect(DirectXCommon::PostEffect effect) {
	DirectXCommon::GetInstance()->SetPostEffect(effect);
}

DirectXCommon::PostEffect SceneManager::GetPostEffect() const {
	return DirectXCommon::GetInstance()->GetPostEffect();
}

void SceneManager::SetFullScreenGray(bool isGray) {
	DirectXCommon::GetInstance()->SetFullScreenGray(isGray);
}

bool SceneManager::IsFullScreenGray() const {
	return DirectXCommon::GetInstance()->IsFullScreenGray();
}

void SceneManager::SetFullScreenVignette(bool isVignette) {
	DirectXCommon::GetInstance()->SetFullScreenVignette(isVignette);
}

bool SceneManager::IsFullScreenVignette() const {
	return DirectXCommon::GetInstance()->IsFullScreenVignette();
}

void SceneManager::SetRadialBlurCenter(const Vector2& center) {
	DirectXCommon::GetInstance()->SetRadialBlurCenter(center);
}

Vector2 SceneManager::GetRadialBlurCenter() const {
	return DirectXCommon::GetInstance()->GetRadialBlurCenter();
}

void SceneManager::SetRadialBlurWidth(float blurWidth) {
	DirectXCommon::GetInstance()->SetRadialBlurWidth(blurWidth);
}

float SceneManager::GetRadialBlurWidth() const {
	return DirectXCommon::GetInstance()->GetRadialBlurWidth();
}

void SceneManager::SetDissolveThreshold(float threshold) {
	DirectXCommon::GetInstance()->SetDissolveThreshold(threshold);
}

float SceneManager::GetDissolveThreshold() const {
	return DirectXCommon::GetInstance()->GetDissolveThreshold();
}

void SceneManager::SetDissolveEdgeColor(const Vector3& color) {
	DirectXCommon::GetInstance()->SetDissolveEdgeColor(color);
}

Vector3 SceneManager::GetDissolveEdgeColor() const {
	return DirectXCommon::GetInstance()->GetDissolveEdgeColor();
}

void SceneManager::SetDissolveEdgeWidth(float width) {
	DirectXCommon::GetInstance()->SetDissolveEdgeWidth(width);
}

float SceneManager::GetDissolveEdgeWidth() const {
	return DirectXCommon::GetInstance()->GetDissolveEdgeWidth();
}

void SceneManager::SetNoiseAlpha(float alpha) {
	DirectXCommon::GetInstance()->SetNoiseAlpha(alpha);
}

float SceneManager::GetNoiseAlpha() const {
	return DirectXCommon::GetInstance()->GetNoiseAlpha();
}

void SceneManager::SetHSVHueShift(float hueShift) {
	DirectXCommon::GetInstance()->SetHSVHueShift(hueShift);
}

float SceneManager::GetHSVHueShift() const {
	return DirectXCommon::GetInstance()->GetHSVHueShift();
}

void SceneManager::SetHSVSaturationMultiplier(float satMult) {
	DirectXCommon::GetInstance()->SetHSVSaturationMultiplier(satMult);
}

float SceneManager::GetHSVSaturationMultiplier() const {
	return DirectXCommon::GetInstance()->GetHSVSaturationMultiplier();
}

void SceneManager::SetHSVValueMultiplier(float valMult) {
	DirectXCommon::GetInstance()->SetHSVValueMultiplier(valMult);
}

float SceneManager::GetHSVValueMultiplier() const {
	return DirectXCommon::GetInstance()->GetHSVValueMultiplier();
}
