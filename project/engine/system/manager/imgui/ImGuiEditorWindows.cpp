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

		int currentPostEffect = static_cast<int>(sceneManager->GetPostEffect());
		int postEffectCount = sizeof(postEffectNames) / sizeof(postEffectNames[0]);
		if (ImGui::Combo("PostEffect", &currentPostEffect, postEffectNames, postEffectCount)) {
			sceneManager->SetPostEffect(static_cast<DirectXCommon::PostEffect>(currentPostEffect));
		}

		// ポストエフェクトごとの設定を表示
		switch (currentPostEffect) {
		case static_cast<int>(DirectXCommon::PostEffect::kFullScreen):
		{
			bool isGray = sceneManager->IsFullScreenGray();
			if (ImGui::Checkbox("FullScreen Gray", &isGray)) {
				sceneManager->SetFullScreenGray(isGray);
			}

			bool isVignette = sceneManager->IsFullScreenVignette();
			if (ImGui::Checkbox("FullScreen Vignette", &isVignette)) {
				sceneManager->SetFullScreenVignette(isVignette);
			}
			break;
		}
		case static_cast<int>(DirectXCommon::PostEffect::kBoxFilter):
			break;
		case static_cast<int>(DirectXCommon::PostEffect::kGaussianFilter):
			break;
		case static_cast<int>(DirectXCommon::PostEffect::kLuminanceBasedOutline):
			break;
		case static_cast<int>(DirectXCommon::PostEffect::kDepthBasedOutline):
			break;
		case static_cast<int>(DirectXCommon::PostEffect::kRadialBlur):
		{
			Vector2 radialBlurCenter = sceneManager->GetRadialBlurCenter();
			float center[2] = { radialBlurCenter.x, radialBlurCenter.y };
			if (ImGui::SliderFloat2("RadialBlur Center", center, 0.0f, 1.0f)) {
				sceneManager->SetRadialBlurCenter({ center[0], center[1] });
			}

			float radialBlurWidth = sceneManager->GetRadialBlurWidth();
			if (ImGui::SliderFloat("RadialBlur Width", &radialBlurWidth, 0.0f, 0.05f)) {
				sceneManager->SetRadialBlurWidth(radialBlurWidth);
			}
			break;
		}
		case static_cast<int>(DirectXCommon::PostEffect::kDissolve):
		{
			float dissolveThreshold = sceneManager->GetDissolveThreshold();
			if (ImGui::SliderFloat("Dissolve Threshold", &dissolveThreshold, 0.0f, 1.0f)) {
				sceneManager->SetDissolveThreshold(dissolveThreshold);
			}

			float dissolveEdgeWidth = sceneManager->GetDissolveEdgeWidth();
			if (ImGui::SliderFloat("Dissolve Edge Width", &dissolveEdgeWidth, 0.0f, 0.2f)) {
				sceneManager->SetDissolveEdgeWidth(dissolveEdgeWidth);
			}

			Vector3 dissolveEdgeColor = sceneManager->GetDissolveEdgeColor();
			float edgeColor[3] = { dissolveEdgeColor.x, dissolveEdgeColor.y, dissolveEdgeColor.z };
			if (ImGui::ColorEdit3("Dissolve Edge Color", edgeColor)) {
				sceneManager->SetDissolveEdgeColor({ edgeColor[0], edgeColor[1], edgeColor[2] });
			}
			break;
		}
		case static_cast<int>(DirectXCommon::PostEffect::kRandomNoise):
		{
			float noiseAlpha = sceneManager->GetNoiseAlpha();
			if (ImGui::SliderFloat("Noise Alpha", &noiseAlpha, 0.0f, 1.0f)) {
				sceneManager->SetNoiseAlpha(noiseAlpha);
			}
			break;
		}
		case static_cast<int>(DirectXCommon::PostEffect::kHSVFilter):
		{
			float hsvHueShift = sceneManager->GetHSVHueShift();
			if (ImGui::SliderFloat("HSV Hue Shift", &hsvHueShift, -1.0f, 1.0f)) {
				sceneManager->SetHSVHueShift(hsvHueShift);
			}
			float hsvSaturationMultiplier = sceneManager->GetHSVSaturationMultiplier();
			if (ImGui::SliderFloat("HSV Saturation Multiplier", &hsvSaturationMultiplier, 0.0f, 2.0f)) {
				sceneManager->SetHSVSaturationMultiplier(hsvSaturationMultiplier);
			}
			float hsvValueMultiplier = sceneManager->GetHSVValueMultiplier();
			if (ImGui::SliderFloat("HSV Value Multiplier", &hsvValueMultiplier, 0.0f, 2.0f)) {
				sceneManager->SetHSVValueMultiplier(hsvValueMultiplier);
			}
			break;
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
