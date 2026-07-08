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

	// ドッキングの有効化
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplWin32_Init(WinApp::GetInstance()->GetHWND());

	// 1. SrvManagerからImGui用のインデックスを安全に確保
	uint32_t imguiSrvIndex = SrvManager::GetInstance()->Allocate();

	// 2. 確保したインデックスからCPU/GPUハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = SrvManager::GetInstance()->GetCPUHandle(imguiSrvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = SrvManager::GetInstance()->GetGPUHandle(imguiSrvIndex);

	// 3. ImGui_ImplDX12_InitInfo構造体を使用して初期化
	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = DirectXCommon::GetInstance()->GetDevice();
	initInfo.CommandQueue = DirectXCommon::GetInstance()->GetCommandQueue();
	initInfo.NumFramesInFlight = 2;
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.SrvDescriptorHeap = DirectXCommon::GetInstance()->GetSRVDescriptorHeap();
	initInfo.LegacySingleSrvCpuDescriptor = cpuHandle;
	initInfo.LegacySingleSrvGpuDescriptor = gpuHandle;

	ImGui_ImplDX12_Init(&initInfo);
#endif
}
void ImGuiManager::NewFrame() {
#ifdef USE_IMGUI
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();

	// 全画面対応のドックスペースをメインビューポートに作成
	ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
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