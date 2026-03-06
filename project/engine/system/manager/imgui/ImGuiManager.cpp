#include "ImGuiManager.h"

#include"BonjinEngine.h"

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

ImGuiManager::ImGuiManager() {
}


ImGuiManager::~ImGuiManager() {

}

void ImGuiManager::Initialize() {
#ifdef USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(WinApp::GetInstance()->GetHWND());
	ImGui_ImplDX12_Init(
		DirectXCommon::GetInstance()->GetDevice(),
		2,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DirectXCommon::GetInstance()->GetSRVDescriptorHeap(),
		DirectXCommon::GetInstance()->GetSRVDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
		DirectXCommon::GetInstance()->GetSRVDescriptorHeap()->GetGPUDescriptorHandleForHeapStart()
	);
#endif
}

void ImGuiManager::NewFrame() {
#ifdef USE_IMGUI
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
#endif
}

void ImGuiManager::EndFrame() {
#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), DirectXCommon::GetInstance()->GetCommandList());
#endif
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	// シングルトンインスタンスをnullptrに戻す
#endif
}