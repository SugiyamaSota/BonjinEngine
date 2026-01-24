#include "LightManager.h"
#include "function/function.h" // CreateBufferResourceがある想定
#include "ImGuiManager.h"

LightManager* LightManager::instance_ = nullptr;

LightManager* LightManager::GetInstance() {
	if (!instance_) instance_ = new LightManager();
	return instance_;
}

void LightManager::DestroyInstance() {
	if (instance_) {
		delete instance_;
		instance_ = nullptr;
	}
}

void LightManager::Initialize(ID3D12Device* device) {
	// リソース作成
	directionalLightResource_ = CreateBufferResource(device, sizeof(DirectionalLight));
	directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

	// 初期値設定
	directionalLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData_->direction = { 0.5f, -1.0f, 0.0f };
	directionalLightData_->intentity = 1.0f;

	// リソース作成
	pointLightResource_ = CreateBufferResource(device, sizeof(PointLight));
	pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

	// 初期値設定
	pointLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pointLightData_->position = { 0.f, 2.f, 0.f };
	pointLightData_->intensity = 1.0f;
	pointLightData_->radius = 5.f;
	pointLightData_->decay = 2.f;
}

void LightManager::Update() {
	
}

void LightManager::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Directional Light")) {
		ImGui::ColorEdit4("Directional_Color", &directionalLightData_->color.x);
		ImGui::DragFloat3("Directional_Direction", &directionalLightData_->direction.x, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("Directional_Intensity", &directionalLightData_->intentity, 0.01f, 0.0f, 10.0f);
		ImGui::TreePop();
	}
#endif

#ifdef USE_IMGUI
	if (ImGui::TreeNode("point Light")) {
		ImGui::ColorEdit4("point_Color", &pointLightData_->color.x);
		ImGui::DragFloat3("point_Position", &pointLightData_->position.x, 0.01f, -10.0f, 10.f);
		ImGui::DragFloat("point_Intensity", &pointLightData_->intensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("point_Radius", &pointLightData_->radius, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("point_Decay", &pointLightData_->decay, 0.01f, 0.0f, 10.0f);
		ImGui::TreePop();
	}
#endif
}