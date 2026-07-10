#include "CameraController.h"
#include "vector.h"
#include "ImGuiManager.h"
#include <cmath>

namespace Bonjin {

void CameraController::Initialize(Camera* camera, const Vector3& initialTarget) {
	camera_ = camera;
	currentTarget_ = initialTarget;
	if (camera_) {
		camera_->SetTarget(currentTarget_);
	}
}

void CameraController::Update(const Vector3& destinationTarget, float deltaTime) {
	if (!camera_) return;

	if (isLerpEnabled_) {
		// フレームレートに依存しない線形補間
		// t = 1.0f - exp(-lerpFactor * deltaTime)
		float t = 1.0f - std::exp(-lerpFactor_ * deltaTime);
		// 念のためのクランプ
		if (t > 1.0f) t = 1.0f;
		if (t < 0.0f) t = 0.0f;

		currentTarget_ = Lerp(currentTarget_, destinationTarget, t);
	} else {
		// 補間なし（即時追従）
		currentTarget_ = destinationTarget;
	}

	camera_->SetTarget(currentTarget_);
}

void CameraController::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Camera Controller")) {
		ImGui::Checkbox("Enable Lerp", &isLerpEnabled_);
		ImGui::SliderFloat("Lerp Factor (Speed)", &lerpFactor_, 0.1f, 20.0f, "%.1f");
		ImGui::Text("Current Target: (%.2f, %.2f, %.2f)", currentTarget_.x, currentTarget_.y, currentTarget_.z);
		if (ImGui::Button("Reset to Target Instant")) {
			if (camera_) {
				currentTarget_ = camera_->GetWorldPosition(); // またはプレイヤー位置へ強制設定するためのボタン
			}
		}
		ImGui::TreePop();
	}
#endif
}

}
