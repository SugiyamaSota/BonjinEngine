#include "SceneManager.h"
#include "time/Time.h" // デルタタイムを取得するため（Timeクラスのパスは適宜修正）
#include "DirectXCommon.h"
#include "SrvManager.h"

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
	if (ImGui::Begin("System Settings")) {
		const char* postEffectNames[] = {
			"FullScreen",
			"BoxFilter",
			"GaussianFilter",
			"LuminanceBasedOutline",
			"DepthBasedOutline",
			"RadialBlur",
			"Dissolve",
			"RandomNoise"
		};

		int currentPostEffect = static_cast<int>(GetPostEffect());
		if (ImGui::Combo("PostEffect", &currentPostEffect, postEffectNames, _countof(postEffectNames))) {
			SetPostEffect(static_cast<DirectXCommon::PostEffect>(currentPostEffect));
		}

		if (ImGui::BeginCombo("Scene", currentScene_->GetScenename())) {
			for (const auto& [sceneType, scene] : scenes_) {
				const bool isSelected = scene.get() == currentScene_;
				if (ImGui::Selectable(scene->GetScenename(), isSelected)) {
					RequestSceneChange(sceneType);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Restart Scene (F5)")) {
			RequestSceneRestart();
		}

		bool isGray = IsFullScreenGray();
		if (ImGui::Checkbox("FullScreen Gray", &isGray)) {
			SetFullScreenGray(isGray);
		}

		bool isVignette = IsFullScreenVignette();
		if (ImGui::Checkbox("FullScreen Vignette", &isVignette)) {
			SetFullScreenVignette(isVignette);
		}

		Vector2 radialBlurCenter = GetRadialBlurCenter();
		float center[2] = { radialBlurCenter.x, radialBlurCenter.y };
		if (ImGui::SliderFloat2("RadialBlur Center", center, 0.0f, 1.0f)) {
			SetRadialBlurCenter({ center[0], center[1] });
		}

		float radialBlurWidth = GetRadialBlurWidth();
		if (ImGui::SliderFloat("RadialBlur Width", &radialBlurWidth, 0.0f, 0.05f)) {
			SetRadialBlurWidth(radialBlurWidth);
		}

		float dissolveThreshold = GetDissolveThreshold();
		if (ImGui::SliderFloat("Dissolve Threshold", &dissolveThreshold, 0.0f, 1.0f)) {
			SetDissolveThreshold(dissolveThreshold);
		}

		float dissolveEdgeWidth = GetDissolveEdgeWidth();
		if (ImGui::SliderFloat("Dissolve Edge Width", &dissolveEdgeWidth, 0.0f, 0.2f)) {
			SetDissolveEdgeWidth(dissolveEdgeWidth);
		}

		Vector3 dissolveEdgeColor = GetDissolveEdgeColor();
		float edgeColor[3] = { dissolveEdgeColor.x, dissolveEdgeColor.y, dissolveEdgeColor.z };
		if (ImGui::ColorEdit3("Dissolve Edge Color", edgeColor)) {
			SetDissolveEdgeColor({ edgeColor[0], edgeColor[1], edgeColor[2] });
		}

		float noiseAlpha = GetNoiseAlpha();
		if (ImGui::SliderFloat("Noise Alpha", &noiseAlpha, 0.0f, 1.0f)) {
			SetNoiseAlpha(noiseAlpha);
		}
	}
	ImGui::End();

	// --- ゲーム画面 ＆ シーン切り替え統合ウィンドウ ---
	if (ImGui::Begin("Game View")) {
		if (ImGui::BeginTabBar("SceneTabs")) {
			// シーンが切り替わった瞬間を検知するための静的変数
			static SceneType lastSceneType = "Exit";
			SceneType currentType = currentScene_->GetCurrentSceneType();
			bool needForceSelect = (lastSceneType != currentType);
			lastSceneType = currentType;

			// タブを描画し、選択されたタブの内部にゲーム画面を描画するラムダ関数
			auto drawTab = [this, currentType, needForceSelect](const char* label, SceneType targetType) {
				bool isCurrent = (currentType == targetType);
				// シーンが切り替わった最初の1フレームのみ強制選択フラグを立てる
				ImGuiTabItemFlags flags = (isCurrent && needForceSelect) ? ImGuiTabItemFlags_SetSelected : 0;
				
				if (ImGui::BeginTabItem(label, nullptr, flags)) {
					// 強制同期中(!needForceSelect)以外で、別タブが選択されたら遷移
					if (!isCurrent && !needForceSelect && !hasPendingSceneChange_) {
						RequestSceneChange(targetType);
					}

					// タブのコンテンツ領域内にゲーム画像を描画
					ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
					DirectXCommon* dxCommon = DirectXCommon::GetInstance();
					RenderTexture* renderTexture = dxCommon->GetRenderTexture();
					if (renderTexture) {
						D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = SrvManager::GetInstance()->GetGPUHandle(renderTexture->GetSrvIndex());
						ImGui::Image(static_cast<ImTextureID>(srvHandle.ptr), viewportPanelSize);
					}

					ImGui::EndTabItem();
				}
			};

			for (const auto& [sceneType, scene] : scenes_) {
				drawTab(scene->GetScenename(), sceneType);
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();

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
