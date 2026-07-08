#include "IScene.h"
#include <algorithm>

using namespace Bonjin;

void IScene::RegisterObject(BaseObject* obj) {
	sceneObjects_.push_back(obj);
}

void IScene::UnregisterObject(BaseObject* obj) {
	auto it = std::find(sceneObjects_.begin(), sceneObjects_.end(), obj);
	if (it != sceneObjects_.end()) {
		sceneObjects_.erase(it);
	}
}

void IScene::Initialize(Camera* camera) {
	assert(camera);
	camera_ = camera;
}

void IScene::DrawImGui() {
#ifdef USE_IMGUI
	// 各シーンでドッキング位置やウィンドウサイズの設定を共有するため、共通のウィンドウ名を使用します
	ImGui::Begin("Scene Settings");

	ImGui::Text("Active Scene: %s", GetScenename());
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Separator();

	DrawSceneImGui();

	ImGui::End();
#endif
}

