#include "BonjinEngine.h"
#include<chrono>
#include<fstream>

#include"ParticleManager.h"

// DirectX関連のライブラリリンク
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"Dbghelp.lib")

// 現在時刻を取得
std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
// 日本時間に変換
std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };
// formatを使って年月日_時分秒の文字列に変換
std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
// 時刻を使ってファイル名決定
std::string logFilePath = std::string("logs/") + dateString + ".log";
// ファイルを使って書き込み準備
std::ofstream logStream(logFilePath);

void Bonjin::Initialize() {
	// directXcommon、Input、テクスチャのインスタンスを取得
	DirectXCommon::GetInstance();
	LightManager::GetInstance()->Initialize(DirectXCommon::GetInstance()->GetDevice());
	WinApp::GetInstance();
	Input::GetInstance()->Initialize(WinApp::GetInstance()->GetHInstance(), WinApp::GetInstance()->GetHWND());
	Input::GetInstance()->SetMouseLock(false);
	SrvManager::GetInstance()->Initialize();
	ModelManager::GetInstance();
	TextureManager::GetInstance();
	ImGuiManager::GetInstance()->Initialize();
	Time::GetInstance();
}

void Bonjin::Finalize() {
	Time::DestroyInstance();
	ImGuiManager::GetInstance()->Finalize();
	ParticleManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	WinApp::GetInstance()->DestroyInstance();
	LightManager::GetInstance()->Finalize();
	DirectXCommon::GetInstance()->DestroyInstance();
}

void Bonjin::NewFrame() {
	Time::GetInstance()->Update();
	Input::GetInstance()->Update();
	ID3D12DescriptorHeap* descriptorHeaps[] = { DirectXCommon::GetInstance()->GetSRVDescriptorHeap() };
	DirectXCommon::GetInstance()->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	ImGuiManager::GetInstance()->NewFrame();
}

void Bonjin::PreDraw() {
	DirectXCommon::GetInstance()->PreDraw();
}

void Bonjin::PostDraw() {
	DirectXCommon::GetInstance()->PostDraw();
}

void Bonjin::EndFrame() {
	ImGuiManager::GetInstance()->EndFrame();
	DirectXCommon::GetInstance()->EndFrame();
}
