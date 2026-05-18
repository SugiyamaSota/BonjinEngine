#include "DirectXCommon.h"
#include<cassert>
#include<filesystem>
#include<thread>

#include"function/function.h"
#include"windows/WinApp.h"
#include"ImGuiManager.h"
#include "SrvManager.h"

DirectXCommon* DirectXCommon::instance_ = nullptr;

DirectXCommon* DirectXCommon::GetInstance() {
	if (instance_ == nullptr) {
		// 初回呼び出し時のみインスタンスを生成
		instance_ = new DirectXCommon();
		instance_->Initialize(); // 必要であればInitializeを呼び出す
	}
	return instance_;
}
void DirectXCommon::DestroyInstance() {
	delete instance_;
	instance_ = nullptr;
}

Microsoft::WRL::ComPtr<ID3D12Resource>  DirectXCommon::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
	//生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	//深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(resource.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return resource;
}

DirectXCommon::DirectXCommon() {
}

void DirectXCommon::Initialize() {
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
		//デバックレイヤーを有効化する
		debugController->EnableDebugLayer();
		//さらにGPUでもチェックを行う
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// 諸々の初期化
	CreateDevice();
	CreateCommand();
	CreateSwapChain();
	CreateFence();
	CreateDepth();
	InitializeFixFPS();

	const Vector4 kRenderTargetClearValue{ 1.f,0.f,0.f,1.f };
	renderTextureResource_ = CreateRenderTextureResource(
		WinApp::GetInstance()->GetClientWidth(),
		WinApp::GetInstance()->GetClientHeight(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		kRenderTargetClearValue
	);
	rtvHandles_[2] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), descriptorSizeRTV_, 2);

	device_->CreateRenderTargetView(renderTextureResource_.Get(), &rtvDesc_, rtvHandles_[2]);

	SrvManager* srvManager = SrvManager::GetInstance();
	renderTextureSrvIndex_ = srvManager->Allocate();

	// 4. CreateSrvを呼び出してSRVを生成
	// (第4、第5引数はTexture2Dの場合デフォルト値(0)で問題ありません)
	srvManager->CreateSrv(
		renderTextureSrvIndex_,
		renderTextureResource_.Get(),
		SrvType::Texture2D
	);


	pso = std::make_unique<PSOManager>();

	// PSOクラスを使用
	pso->Initialize(
		device_.Get(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DXGI_FORMAT_D24_UNORM_S8_UINT
	);

	viewport_.Width = float(WinApp::GetInstance()->GetClientWidth());
	viewport_.Height = float(WinApp::GetInstance()->GetClientHeight());
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	scissorRect_.left = 0;
	scissorRect_.right = LONG(WinApp::GetInstance()->GetClientWidth());
	scissorRect_.top = 0;
	scissorRect_.bottom = LONG(WinApp::GetInstance()->GetClientHeight());

}

DirectXCommon::~DirectXCommon() {
	if (fenceEvent_ != nullptr) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
}

void DirectXCommon::PreDraw() {
	// 1. まず RenderTexture の状態を RENDER_TARGET に遷移させる
	barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier_.Transition.pResource = renderTextureResource_.Get();
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier_);

	// 2. バリア完了後に、描画ターゲット（RenderTexture）と深度バッファをセット
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->OMSetRenderTargets(1, &rtvHandles_[2], false, &dsvHandle);

	// 3. ディスクリプタヒープの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(1, descriptorHeaps);

	// 4. クリア処理
	float clearColor[] = { 1.f, 0.f, 0.f, 1.0f }; // RenderTextureを赤でクリア
	commandList_->ClearRenderTargetView(rtvHandles_[2], clearColor, 0, nullptr);
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// ビューポートとシザーの設定
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);
}

void DirectXCommon::PostDraw() {
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// -------------------------------------------------------------------------
	// ★修正: 描画先を切り替える「前」に、すべてのリソースバリアを完了させる
	// -------------------------------------------------------------------------

	// A) RenderTexture を 描画ターゲット から シェーダー読み込み用(SRV) に
	D3D12_RESOURCE_BARRIER rtBarrier{};
	rtBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rtBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rtBarrier.Transition.pResource = renderTextureResource_.Get();
	rtBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	rtBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// B) バックバッファを 画面表示用(PRESENT) から 描画ターゲット に
	D3D12_RESOURCE_BARRIER bbBarrier{};
	bbBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bbBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	bbBarrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	bbBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	bbBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	D3D12_RESOURCE_BARRIER barriers[] = { rtBarrier, bbBarrier };
	commandList_->ResourceBarrier(_countof(barriers), barriers);

	// -------------------------------------------------------------------------
	// ★修正: バリア完了後に、初めてバックバッファをターゲットとしてバインドする
	// -------------------------------------------------------------------------
	// フルスクリーンコピーの際は深度テストを行わない（または常に通過する）ため、DSVは nullptr で運用するのが安全です
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, nullptr);

	// バックバッファをクリア（紺色）
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

	// ビューポートとシザー矩形を設定
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);

	// SrvManagerのディスクリプタヒープをセット
	SrvManager::GetInstance()->PreDraw();

	// RenderTextureの内容をバックバッファにフルスクリーンコピー描画
	commandList_->SetGraphicsRootSignature(pso->GetRootSignature(PrimitiveType::kCopyImage));
	commandList_->SetPipelineState(pso->GetPipelineState(
		device_.Get(), PrimitiveType::kCopyImage, BlendMode::kNone, D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE
	));
	commandList_->SetGraphicsRootDescriptorTable(0, SrvManager::GetInstance()->GetGPUHandle(renderTextureSrvIndex_));

	commandList_->DrawInstanced(3, 1, 0, 0); // フルスクリーン三角形で描画
}

void DirectXCommon::EndFrame() 
{	

	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// 状態を遷移（バックバッファをPRESENTに戻す）
	barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier_.Transition.pResource = swapChainResources_[backBufferIndex].Get(); // ★ここを明示
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &barrier_);

	// -------------------------------------------------------------------------
	// ★追加：RenderTexture の状態を次のフレームのために COMMON（または RENDER_TARGET用）に戻しておく
	// -------------------------------------------------------------------------
	barrier_.Transition.pResource = renderTextureResource_.Get();
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
	commandList_->ResourceBarrier(1, &barrier_);

	// コマンドリストの内容を確定させる（既存の処理）
	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));
	//GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);
	//GPUとOSに画面の交換を行うよう通知
	swapChain_->Present(1, 0);

	//Fenceの値を更新
	fenceValue_++;
	//GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようSignalを送る
	commandQueue_->Signal(fence_.Get(), fenceValue_);
	//Fenceの値が指定したsignal値のたどり着いてるか確認
	//GetCompletedValueの初期値はFence作成時に渡した初期値
	if (fence_.Get()->GetCompletedValue() < fenceValue_) {
		//指定したsignakにたどり着いてないので、たどり着くまで待つようにイベントを設定する
		fence_.Get()->SetEventOnCompletion(fenceValue_, fenceEvent_);
		//イベントを待つ
		WaitForSingleObject(fenceEvent_, INFINITE);

	}

	UpdateFixFPS();

	//次のフレーム用のコマンドリストを準備
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::WaitAndResetCommandList() {
	HRESULT hr;

	// commandListをcloseし、キックする
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	// Fenceの値をインクリメント
	UINT64 currentFenceValue = fenceValue_; // シグナルを送る前のフェンス値
	IncrementFencevalue(); // 次のフレーム用のフェンス値に更新

	// コマンドキューにFenceのシグナルを送る
	hr = commandQueue_->Signal(fence_.Get(), fenceValue_); // 更新されたフェンス値をシグナルに使う
	assert(SUCCEEDED(hr));

	// 指定したsignalにたどり着いてないので、たどり着くまで待つようにイベントを設定する
	// GetCompletedValue()は、最後に完了したコマンドのフェンス値を取得する
	if (fence_->GetCompletedValue() < fenceValue_) { // currentFenceValueではなく、signalに送ったfenceValue_と比較する
		hr = fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		assert(SUCCEEDED(hr));
		// イベントを待つ
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateDevice() {
	//DXGIファクトリーの作成
	dxgiFactory_ = nullptr;
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	//使用するアダプタ用の変数。最初にnullptrを入れる
	useAdapter_ = nullptr;
	//いい順にアダプタを頼む
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(useAdapter_.GetAddressOf())) != DXGI_ERROR_NOT_FOUND; ++i) {
		//あだぷたーの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter_->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアアダプタでなければ採用!

		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//採用したアダプタの情報をログに出力、
			//Log(logStream, ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter_ = nullptr;
	}
	//適切なアダプタがないので起動不可
	assert(useAdapter_ != nullptr);

	device_ = nullptr;
	//昨日レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLvels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLvels); ++i) {
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter_.Get(), featureLvels[i], IID_PPV_ARGS(device_.GetAddressOf())); // Get() と GetAddressOf() を使用
		//指定した昨日レベルでデバイスが生成できたか確認
		if (SUCCEEDED(hr)) {
			//静背できたのでる＾ぷを抜ける
			//Log(logStream, std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	//デバイス生成がうまくいかず起動できない
	assert(device_ != nullptr);
	//Log(logStream, "Complete create D3D12Device!!!\n");//初期化完了ログ

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(infoQueue.GetAddressOf())))) {
		//やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		//解放 (ComPtr が自動でReleaseするため不要)
		//infoQueue->Release(); 
		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			//Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			//https://stackoverflow.com\questions\69805245\directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
	}
#endif
}

void DirectXCommon::CreateCommand() {
	//コマンドキューの生成
	D3D12_COMMAND_QUEUE_DESC commandQueueDisc{};
	HRESULT hr = device_->CreateCommandQueue(&commandQueueDisc, IID_PPV_ARGS(commandQueue_.GetAddressOf()));
	assert(SUCCEEDED(hr));//コマンドキュー生成の可否

	//コマンドアロケータの生成
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator_.GetAddressOf()));
	//コマンドアロケータがうまく生成できなかった
	assert(SUCCEEDED(hr));

	//コマンドリストの生成
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(commandList_.GetAddressOf())); // Get() と GetAddressOf() を使用
	//コマンドリストがうまく生成できなかった
	assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateSwapChain() {
	//スワップチェーンの生成
	swapChainDesc_.Width = WinApp::GetInstance()->GetClientWidth();
	swapChainDesc_.Height = WinApp::GetInstance()->GetClientHeight();
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc_.SampleDesc.Count = 1;
	swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc_.BufferCount = 2;
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//コマンドキュー、ウィンドウハンドル、設定を渡して生成
	HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), WinApp::GetInstance()->GetHWND(), &swapChainDesc_, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf())); // Get() と GetAddressOf() を使用
	assert(SUCCEEDED(hr));

	//スワップチェーンからリソースを引っ張る
	hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(swapChainResources_[0].GetAddressOf()));
	//うまく取得できないと起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(swapChainResources_[1].GetAddressOf()));
	assert(SUCCEEDED(hr));

	//ディスクリプタヒープの生成
	//ディスクリプタサイズを取得する
	descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//RTVの設定
	rtvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);
	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//出力結果をSRGBに変換
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2dテクスチャとして書き込む
	//ディスクリプらの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	//RTVを2ツ作るのでディスクリプタを2つ用意
	//まず一つ目を作る。一つ目は最初のところに作る。作る場所をこっちで指定する
	rtvHandles_[0] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), descriptorSizeRTV_, 0);
	device_.Get()->CreateRenderTargetView(swapChainResources_[0].Get(), &rtvDesc_, rtvHandles_[0]);
	//2つ目のディスクリプタハンドルを得る(自力で)
	rtvHandles_[1] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), descriptorSizeRTV_, 1);
	//二つ目を得る
	device_.Get()->CreateRenderTargetView(swapChainResources_[1].Get(), &rtvDesc_, rtvHandles_[1]);

	srvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GetSRVSize() * 100, true);
}

void DirectXCommon::CreateFence() {
	//初期値0でFenceを作る

	HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	//FenceのSignalを持つためのイベントを作成する
	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent_ != nullptr);
}

void DirectXCommon::CreateDepth() {
	//DepthStencilTextureをウィンドウのサイズで作成
	depthStencilResource_ = CreateDepthStencilTextureResource(device_.Get(), WinApp::GetInstance()->GetClientWidth(), WinApp::GetInstance()->GetClientHeight());
	//DSV
	dsvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	//DSVの設定
	dsvDesc_.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc_.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//DSVHeapの先頭にDSVをつくる
	device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc_, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

	//DepthStencilStateの設定
	//Depthの機能を有効化
	depthStencilDesc_.DepthEnable = true;
	//書き込みします
	depthStencilDesc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数
	depthStencilDesc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

void DirectXCommon::InitializeFixFPS()
{
	reference_ = std::chrono::steady_clock::now();
}

void DirectXCommon::UpdateFixFPS()
{
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.f));

	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.f));

	// 現在時間を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

	// 前回記録からの経過時間を取得する
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	if (elapsed < kMinCheckTime) {
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			//
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	reference_ = std::chrono::steady_clock::now();
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor) {
	//生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	//利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&clearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

