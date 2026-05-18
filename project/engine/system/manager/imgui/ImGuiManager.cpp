#include "ImGuiManager.h"

#include"BonjinEngine.h"
#include "SrvManager.h"

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

	// 1. SrvManagerからImGui用のインデックスを安全に確保
	uint32_t imguiSrvIndex = SrvManager::GetInstance()->Allocate();

	// 2. 確保したインデックスからCPU/GPUハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = SrvManager::GetInstance()->GetCPUHandle(imguiSrvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = SrvManager::GetInstance()->GetGPUHandle(imguiSrvIndex);

	// 3. ImGuiの初期化にハンドルを渡す
	ImGui_ImplDX12_Init(
		DirectXCommon::GetInstance()->GetDevice(),
		2, // バックバッファ数
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DirectXCommon::GetInstance()->GetSRVDescriptorHeap(),
		cpuHandle, // 修正：先頭ではなく専用のハンドル
		gpuHandle  // 修正：先頭ではなく専用のハンドル
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