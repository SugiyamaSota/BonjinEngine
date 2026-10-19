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

	if (transition_) {
		transition_->Reset();
		transition_.reset();
	}
}

void SceneManager::Initialize() {
	camera_ = std::make_unique<Camera>();
	camera_->Initialize(1280, 720);
	transition_ = std::make_unique<SceneTransition>();
	transition_->Initialize();
}

// 💡 4. シーンの登録
void SceneManager::AddScene(SceneType type, std::unique_ptr<IScene> scene) {
	// 既に登録されているかチェック（オプション）
	if (scenes_.find(type) != scenes_.end()) {
		// エラー処理または上書き処理
		return;
	}

	scenes_[type] = std::move(scene);
}

// 💡 5. シーンの更新処理
void SceneManager::Update(float deltaTime) {
	// 1. トランジションの更新
	if (transition_) {
		transition_->Update(deltaTime);

		// シーン切り替え（Unload & Initialize）を実行すべき瞬間
		if (transition_->ShouldSwitchScene()) {
			if (hasPendingSceneChange_) {
				ExecuteSceneChange(pendingSceneType_);
				hasPendingSceneChange_ = false;
			}
			transition_->NotifySceneSwitched();
		}
	}

	if (currentScene_ == nullptr) {
		return;
	}

	// トランジション中でない場合のみリスタート入力を受付
	if (!IsTransitionActive()) {
		if (Input::GetInstance()->IsTrigger(DIK_F5)) {
			RequestSceneRestart();
		}
	}

	if (hasPendingSceneRestart_) {
		hasPendingSceneRestart_ = false;
		RestartCurrentScene();
	}

	camera_->Update(Camera::CameraType::kDebug);

	// 現在のシーンの更新処理を呼び出し（暗転ウェイト中はシーン更新を停止）
	bool shouldUpdateScene = true;
	if (transition_ && transition_->GetPhase() == TransitionPhase::kWait) {
		shouldUpdateScene = false;
	}

	if (shouldUpdateScene) {
		currentScene_->Update(deltaTime);
	}

	// 遷移先のシーンタイプを取得（従来の GetNextScene 方式との互換性）
	if (!IsTransitionActive()) {
		SceneType nextType = currentScene_->GetNextScene();
		if (nextType != currentScene_->GetCurrentSceneType()) {
			RequestSceneChange(nextType);
		}
	}
}

// 💡 6. シーンの描画処理
void SceneManager::Draw() {
	if (currentScene_ == nullptr) {
		return;
	}

	// 暗転ウェイト中（完全ブラックアウト時）は描画をスキップして効率化
	if (transition_ && transition_->GetPhase() == TransitionPhase::kWait) {
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

#include <chrono>
#include <sstream>

void SceneManager::ChangeScene(SceneType nextSceneType) {
	if (transition_) {
		transition_->Reset();
	}
	hasPendingSceneChange_ = false;
	ExecuteSceneChange(nextSceneType);
}

void SceneManager::ExecuteSceneChange(SceneType nextSceneType) {
	if (nextSceneType == "Exit") {
		// 終了処理
		return;
	}

	auto tStart = std::chrono::high_resolution_clock::now();
	std::string prevSceneType = currentScene_ ? currentScene_->GetCurrentSceneType() : "None";

	if (currentScene_ != nullptr) {
		currentScene_->Unload();
	}

	auto tUnload = std::chrono::high_resolution_clock::now();

	auto it = scenes_.find(nextSceneType);
	if (it == scenes_.end()) return;

	// シーン切り替え時にVignette等のエフェクトをデフォルトに戻す
	ResetPostEffects();

	// unique_ptr が管理する実体のアドレスをセット
	currentScene_ = it->second.get();
	currentScene_->Initialize(camera_.get());

	auto tEnd = std::chrono::high_resolution_clock::now();

	float unloadMs = std::chrono::duration<float, std::milli>(tUnload - tStart).count();
	float initMs = std::chrono::duration<float, std::milli>(tEnd - tUnload).count();
	float totalMs = std::chrono::duration<float, std::milli>(tEnd - tStart).count();

	std::stringstream ss;
	ss << "[SceneTransition] " << prevSceneType << " -> " << nextSceneType 
	   << " | Unload: " << unloadMs << " ms | Initialize: " << initMs << " ms | Total: " << totalMs << " ms\n";
	OutputDebugStringA(ss.str().c_str());
}

void SceneManager::RequestSceneChange(SceneType nextSceneType) {
	TransitionDesc desc;

	// 1. 遷移元シーンのアウト演出設定を反映
	if (currentScene_) {
		if (currentScene_->HasCustomTransitionOut()) {
			desc.outType = TransitionType::kCustom;
		} else {
			desc.outType = currentScene_->GetTransitionOutType();
		}
		desc.outDuration = currentScene_->GetTransitionOutDuration();
		desc.waitDuration = currentScene_->GetTransitionWaitDuration();
		desc.edgeColor = currentScene_->GetTransitionEdgeColor();
	}

	// 2. 遷移先シーンのイン演出設定を反映
	auto it = scenes_.find(nextSceneType);
	if (it != scenes_.end() && it->second) {
		if (it->second->HasCustomTransitionIn()) {
			desc.inType = TransitionType::kCustom;
		} else {
			desc.inType = it->second->GetTransitionInType();
		}
		desc.inDuration = it->second->GetTransitionInDuration();
	}

	RequestSceneChange(nextSceneType, desc);
}

void SceneManager::RequestSceneChange(SceneType nextSceneType, const TransitionDesc& desc) {
	if (currentScene_ != nullptr && nextSceneType == currentScene_->GetCurrentSceneType()) {
		return;
	}
	if (IsTransitionActive()) {
		return; // 既に遷移中
	}

	pendingSceneType_ = nextSceneType;
	hasPendingSceneChange_ = true;

	if (transition_) {
		transition_->Start(desc);
	} else {
		ExecuteSceneChange(pendingSceneType_);
		hasPendingSceneChange_ = false;
	}
}

void SceneManager::StartTransition(const TransitionDesc& desc) {
	if (transition_) {
		transition_->Start(desc);
	}
}

bool SceneManager::IsTransitionActive() const {
	return transition_ && transition_->IsActive();
}

void SceneManager::RestartCurrentScene() {
	if (currentScene_ == nullptr) {
		return;
	}

	currentScene_->Unload();

	// リスタート時にVignette等のエフェクトをデフォルトに戻す
	ResetPostEffects();

	currentScene_->Initialize(camera_.get());
}

void SceneManager::RequestSceneRestart() {
	if (currentScene_ == nullptr || hasPendingSceneChange_ || IsTransitionActive()) {
		return;
	}

	hasPendingSceneRestart_ = true;
}

void SceneManager::SetPostEffect(PostEffect effect) {
	DirectXCommon::GetInstance()->SetPostEffect(effect);
}

PostEffect SceneManager::GetPostEffect() const {
	return DirectXCommon::GetInstance()->GetPostEffect();
}

void SceneManager::ClearPostEffects() {
	DirectXCommon::GetInstance()->ClearPostEffects();
}

void SceneManager::ResetPostEffects() {
	SetFullScreenVignette(false);
	SetFullScreenVignetteColor({ 0.0f, 0.0f, 0.0f });
	SetFullScreenVignetteScale(16.0f);
	SetFullScreenVignettePower(0.8f);
	SetCRTShutDownProgress(0.0f);
}

void SceneManager::AddPostEffect(PostEffect effect) {
	DirectXCommon::GetInstance()->AddPostEffect(effect);
}

void SceneManager::RemovePostEffect(PostEffect effect) {
	DirectXCommon::GetInstance()->RemovePostEffect(effect);
}

void SceneManager::SetActiveEffects(const std::vector<PostEffect>& effects) {
	DirectXCommon::GetInstance()->SetActiveEffects(effects);
}

const std::vector<PostEffect>& SceneManager::GetActiveEffects() const {
	return DirectXCommon::GetInstance()->GetActiveEffects();
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

void SceneManager::SetFullScreenVignetteColor(const Vector3& color) {
	DirectXCommon::GetInstance()->SetFullScreenVignetteColor(color);
}

Vector3 SceneManager::GetFullScreenVignetteColor() const {
	return DirectXCommon::GetInstance()->GetFullScreenVignetteColor();
}

void SceneManager::SetFullScreenVignetteScale(float scale) {
	DirectXCommon::GetInstance()->SetFullScreenVignetteScale(scale);
}

float SceneManager::GetFullScreenVignetteScale() const {
	return DirectXCommon::GetInstance()->GetFullScreenVignetteScale();
}

void SceneManager::SetFullScreenVignettePower(float power) {
	DirectXCommon::GetInstance()->SetFullScreenVignettePower(power);
}

float SceneManager::GetFullScreenVignettePower() const {
	return DirectXCommon::GetInstance()->GetFullScreenVignettePower();
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

void SceneManager::SetFadeAmount(float amount) {
	DirectXCommon::GetInstance()->SetFadeAmount(amount);
}

float SceneManager::GetFadeAmount() const {
	return DirectXCommon::GetInstance()->GetFadeAmount();
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

void SceneManager::SetCRTShutDownProgress(float progress) {
	DirectXCommon::GetInstance()->SetCRTShutDownProgress(progress);
}

float SceneManager::GetCRTShutDownProgress() const {
	return DirectXCommon::GetInstance()->GetCRTShutDownProgress();
}
