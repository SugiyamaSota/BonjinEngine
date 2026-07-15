#include "ImGuiEditorWindows.h"
#include "SceneManager.h"
#include "../../../interface/IScene.h"
#include "BaseObject.h"
#include "SrvManager.h"
#include "DirectXCommon.h"
#include "../../externals/imgui/imgui.h"

using namespace Bonjin;

void ImGuiEditorWindows::DrawSystemSettings(SceneManager* sceneManager) {
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
			"RandomNoise",
			"HSVFilter"
		};

		if (ImGui::Button("Restart Scene (F5)")) {
			sceneManager->RequestSceneRestart();
		}

		// ポストエフェクト重ね掛け用のエディタUI
		ImGui::Text("Active Post Effects:");
		auto activeEffects = sceneManager->GetActiveEffects();
		std::vector<DirectXCommon::PostEffect> nextEffects = activeEffects;

		int removeIndex = -1;
		int moveUpIndex = -1;
		int moveDownIndex = -1;

		ImGui::BeginChild("EffectsList", ImVec2(0, 150), true);
		for (size_t i = 0; i < nextEffects.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			ImGui::Text("[%d] %s", static_cast<int>(i), postEffectNames[static_cast<int>(nextEffects[i])]);
			ImGui::SameLine();
			if (ImGui::Button("Up") && i > 0) {
				moveUpIndex = static_cast<int>(i);
			}
			ImGui::SameLine();
			if (ImGui::Button("Down") && i < nextEffects.size() - 1) {
				moveDownIndex = static_cast<int>(i);
			}
			ImGui::SameLine();
			if (ImGui::Button("Remove")) {
				removeIndex = static_cast<int>(i);
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		if (removeIndex != -1) {
			nextEffects.erase(nextEffects.begin() + removeIndex);
		}
		if (moveUpIndex != -1) {
			std::swap(nextEffects[moveUpIndex], nextEffects[moveUpIndex - 1]);
		}
		if (moveDownIndex != -1) {
			std::swap(nextEffects[moveDownIndex], nextEffects[moveDownIndex + 1]);
		}

		// 追加用Combo
		static int selectedEffectToAdd = 0;
		int postEffectCount = sizeof(postEffectNames) / sizeof(postEffectNames[0]);
		ImGui::Combo("Add Effect", &selectedEffectToAdd, postEffectNames, postEffectCount);
		ImGui::SameLine();
		if (ImGui::Button("Add")) {
			nextEffects.push_back(static_cast<DirectXCommon::PostEffect>(selectedEffectToAdd));
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear All")) {
			nextEffects.clear();
		}

		// 変更があれば適用
		if (nextEffects != activeEffects) {
			sceneManager->SetActiveEffects(nextEffects);
		}

		ImGui::Separator();

		// ポストエフェクトごとの設定を CollapsingHeader で表示
		if (ImGui::CollapsingHeader("FullScreen Settings")) {
			bool isGray = sceneManager->IsFullScreenGray();
			if (ImGui::Checkbox("Gray Scale", &isGray)) {
				sceneManager->SetFullScreenGray(isGray);
			}
			bool isVignette = sceneManager->IsFullScreenVignette();
			if (ImGui::Checkbox("Vignette", &isVignette)) {
				sceneManager->SetFullScreenVignette(isVignette);
			}
		}

		if (ImGui::CollapsingHeader("RadialBlur Settings")) {
			Vector2 radialBlurCenter = sceneManager->GetRadialBlurCenter();
			float center[2] = { radialBlurCenter.x, radialBlurCenter.y };
			if (ImGui::SliderFloat2("Center", center, 0.0f, 1.0f)) {
				sceneManager->SetRadialBlurCenter({ center[0], center[1] });
			}
			float radialBlurWidth = sceneManager->GetRadialBlurWidth();
			if (ImGui::SliderFloat("Width", &radialBlurWidth, 0.0f, 0.05f)) {
				sceneManager->SetRadialBlurWidth(radialBlurWidth);
			}
		}

		if (ImGui::CollapsingHeader("Dissolve Settings")) {
			float dissolveThreshold = sceneManager->GetDissolveThreshold();
			if (ImGui::SliderFloat("Threshold", &dissolveThreshold, 0.0f, 1.0f)) {
				sceneManager->SetDissolveThreshold(dissolveThreshold);
			}
			float dissolveEdgeWidth = sceneManager->GetDissolveEdgeWidth();
			if (ImGui::SliderFloat("Edge Width", &dissolveEdgeWidth, 0.0f, 0.2f)) {
				sceneManager->SetDissolveEdgeWidth(dissolveEdgeWidth);
			}
			Vector3 dissolveEdgeColor = sceneManager->GetDissolveEdgeColor();
			float edgeColor[3] = { dissolveEdgeColor.x, dissolveEdgeColor.y, dissolveEdgeColor.z };
			if (ImGui::ColorEdit3("Edge Color", edgeColor)) {
				sceneManager->SetDissolveEdgeColor({ edgeColor[0], edgeColor[1], edgeColor[2] });
			}
		}

		if (ImGui::CollapsingHeader("RandomNoise Settings")) {
			float noiseAlpha = sceneManager->GetNoiseAlpha();
			if (ImGui::SliderFloat("Noise Alpha", &noiseAlpha, 0.0f, 1.0f)) {
				sceneManager->SetNoiseAlpha(noiseAlpha);
			}
		}

		if (ImGui::CollapsingHeader("HSVFilter Settings")) {
			float hsvHueShift = sceneManager->GetHSVHueShift();
			if (ImGui::SliderFloat("Hue Shift", &hsvHueShift, -1.0f, 1.0f)) {
				sceneManager->SetHSVHueShift(hsvHueShift);
			}
			float hsvSaturationMultiplier = sceneManager->GetHSVSaturationMultiplier();
			if (ImGui::SliderFloat("Saturation Multiplier", &hsvSaturationMultiplier, 0.0f, 2.0f)) {
				sceneManager->SetHSVSaturationMultiplier(hsvSaturationMultiplier);
			}
			float hsvValueMultiplier = sceneManager->GetHSVValueMultiplier();
			if (ImGui::SliderFloat("Value Multiplier", &hsvValueMultiplier, 0.0f, 2.0f)) {
				sceneManager->SetHSVValueMultiplier(hsvValueMultiplier);
			}
		}
	}
	ImGui::End();
#endif
}

void ImGuiEditorWindows::DrawGameView(SceneManager* sceneManager) {
#ifdef USE_IMGUI
	if (ImGui::Begin("Game View")) {
		if (ImGui::BeginTabBar("SceneTabs")) {
			static SceneType lastSceneType = "Exit";
			SceneType currentType = sceneManager->GetCurrentScene()->GetCurrentSceneType();
			bool needForceSelect = (lastSceneType != currentType);
			lastSceneType = currentType;

			auto drawTab = [sceneManager, currentType, needForceSelect](const char* label, SceneType targetType) {
				bool isCurrent = (currentType == targetType);
				ImGuiTabItemFlags flags = (isCurrent && needForceSelect) ? ImGuiTabItemFlags_SetSelected : 0;
				
				if (ImGui::BeginTabItem(label, nullptr, flags)) {
					if (!isCurrent && !needForceSelect && !sceneManager->HasPendingSceneChange()) {
						sceneManager->RequestSceneChange(targetType);
					}

					ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
					DirectXCommon* dxCommon = DirectXCommon::GetInstance();
					RenderTexture* renderTexture = dxCommon->GetPostEffectTexture();
					if (renderTexture) {
						D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = SrvManager::GetInstance()->GetGPUHandle(renderTexture->GetSrvIndex());
						ImGui::Image(static_cast<ImTextureID>(srvHandle.ptr), viewportPanelSize);
					}

					ImGui::EndTabItem();
				}
			};

			for (const auto& [sceneType, scene] : sceneManager->GetScenes()) {
				drawTab(scene->GetScenename(), sceneType);
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
#endif
}

void ImGuiEditorWindows::DrawHierarchy(IScene* currentScene) {
#ifdef USE_IMGUI
	if (ImGui::Begin("Hierarchy")) {
		for (BaseObject* obj : currentScene->GetSceneObjects()) {
			if (ImGui::TreeNode(obj, "%s", obj->GetName().c_str())) {
				obj->DrawImGui(obj->GetName());
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
#endif
}
