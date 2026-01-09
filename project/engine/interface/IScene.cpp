#include"IScene.h"

using namespace Bonjin;

void IScene::Initialize(Camera* camera) {
	assert(camera);
	camera_ = camera;
}

void IScene::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin(GetScenename());

	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

	DrawSceneImGui();

	ImGui::End();
#endif
}

