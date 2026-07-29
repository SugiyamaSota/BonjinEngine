#include "DirectXCommon.h"
#include<algorithm>
#include<cassert>
#include<filesystem>
#include<thread>

#include "function/function.h"
#include "windows/WinApp.h"
#include "ImGuiManager.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "math/Matrix.h"
#include "SceneManager.h"
#include "time/Time.h"

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

DirectXCommon::DirectXCommon() {
}

void DirectXCommon::Initialize() {
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
		//デバックレイヤーを有効化する
		debugController->EnableDebugLayer();
		//さらにGPUでもチェックを行う (※非常に重いため通常はコメントアウト)
		//debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// 諸々の初期化
	CreateDevice();
	CreateCommand();
	CreateSwapChain();
	CreateFence();

	// DepthStencilクラスの生成
	depthStencil_ = std::make_unique<DepthStencil>(device_.Get());

	InitializeFixFPS();

	// 既存のコードを削除し、以下に置き換え
	rtvHandles_[2] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), descriptorSizeRTV_, 2);
	rtvHandles_[3] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), descriptorSizeRTV_, 3);
	rtvHandles_[4] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), descriptorSizeRTV_, 4);

	// RenderTextureクラスの生成にRTVハンドルを渡す
	renderTexture_ = std::make_unique<RenderTexture>(device_.Get(), rtvHandles_[2]);
	tempRenderTexture_ = std::make_unique<RenderTexture>(device_.Get(), rtvHandles_[4]);
	postEffectTexture_ = std::make_unique<RenderTexture>(device_.Get(), rtvHandles_[3]);


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

	// FullScreen/DepthOutline 用の定数バッファを生成
	fullScreenMaterial_.projectionInverse = MakeIdentity4x4();
	fullScreenMaterial_.isGray = true;
	fullScreenMaterial_.isVignette = false;
	fullScreenMaterial_.radialBlurCenter = { 0.5f, 0.5f };
	fullScreenMaterial_.radialBlurWidth = 0.01f;
	fullScreenMaterial_.dissolveThreshold = 0.5f;
	fullScreenMaterial_.dissolveEdgeColor = { 1.0f, 0.4f, 0.3f };
	fullScreenMaterial_.dissolveEdgeWidth = 0.03f;
	fullScreenMaterial_.time = 0.0f;
	fullScreenMaterial_.noiseAlpha = 0.5f;
	fullScreenMaterial_.hsvHueShift = 0.0f;
	fullScreenMaterial_.hsvSaturationMultiplier = 1.0f;
	fullScreenMaterial_.hsvValueMultiplier = 1.0f;
	fullScreenMaterial_.vignetteColor = { 0.0f, 0.0f, 0.0f };
	fullScreenMaterial_.vignetteScale = 16.0f;
	fullScreenMaterial_.vignettePower = 0.8f;
	fullScreenCB_ = CreateBufferResource(device_.Get(), sizeof(FullScreenMaterial));
	fullScreenCB_->Map(0, nullptr, reinterpret_cast<void**>(&fullScreenData_));
	*fullScreenData_ = fullScreenMaterial_;
	dissolveMaskTextureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/textures/noise0.png");

	// RenderTextureとtempRenderTextureとpostEffectTextureを初期ステートであるPIXEL_SHADER_RESOURCEに遷移させておく
	D3D12_RESOURCE_BARRIER initBarriers[3] = {};
	initBarriers[0] = renderTexture_->CreateTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	initBarriers[1] = tempRenderTexture_->CreateTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	initBarriers[2] = postEffectTexture_->CreateTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList_->ResourceBarrier(3, initBarriers);

	// コマンドリストを閉じて実行、同期待ちを行う
	commandList_->Close();
	ID3D12CommandList* cmdLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, cmdLists);
	
	// フェンスで同期を取る
	fenceValue_++;
	commandQueue_->Signal(fence_.Get(), fenceValue_);
	if (fence_->GetCompletedValue() < fenceValue_) {
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// コマンドリストをリセットして次のフレームに備える
	commandAllocator_->Reset();
	commandList_->Reset(commandAllocator_.Get(), nullptr);
}

DirectXCommon::~DirectXCommon() {
	if (fullScreenCB_) {
		fullScreenCB_->Unmap(0, nullptr);
	}
	if (fenceEvent_ != nullptr) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
}

void DirectXCommon::PreDraw() 
{

	ID3D12DescriptorHeap* srvDescriptorHeaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(_countof(srvDescriptorHeaps), srvDescriptorHeaps);

	// 1. まず RenderTexture の状態を RENDER_TARGET に遷移させる
	D3D12_RESOURCE_BARRIER rtBarrier = renderTexture_->CreateTransitionBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList_->ResourceBarrier(1, &rtBarrier);

	// 2. バリア完了後に、描画ターゲット（RenderTexture）と深度バッファをセット
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencil_->GetHandle();
	commandList_->OMSetRenderTargets(1, &rtvHandles_[2], false, &dsvHandle);

	// 3. ディスクリプタヒープの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(1, descriptorHeaps);

	// 4. RenderTextureをクリア
	renderTexture_->ClearView(commandList_.Get());

	// 5. 深度バッファをクリア
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// ビューポートとシザーの設定
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);

}

void DirectXCommon::PostDraw()
{
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// 1. RenderTexture (t0) を 描画ターゲット から ピクセルシェーダー用(SRV) に遷移
	D3D12_RESOURCE_BARRIER rtBarrier = renderTexture_->CreateTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// 2. 深度テクスチャ (t1) を デプス書き込み から ピクセルシェーダー用(SRV) に遷移
	D3D12_RESOURCE_BARRIER depthBarrier{};
	depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	depthBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	depthBarrier.Transition.pResource = depthStencil_->GetResource();
	depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	D3D12_RESOURCE_BARRIER initialBarriers[] = { rtBarrier, depthBarrier };
	commandList_->ResourceBarrier(_countof(initialBarriers), initialBarriers);

	// ビューポートとシザー矩形を設定
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);

	// SrvManagerのディスクリプタヒープをセット
	SrvManager::GetInstance()->PreDraw();

	// 定数バッファの更新
	Matrix4x4 projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(WinApp::GetInstance()->GetClientWidth()) / float(WinApp::GetInstance()->GetClientHeight()), 0.1f, 1000.0f);
	fullScreenMaterial_.projectionInverse = Inverse(projectionMatrix_);
	fullScreenMaterial_.time = Bonjin::Time::GetInstance()->GetElapsedTime();
	*fullScreenData_ = fullScreenMaterial_;

	// 適用するエフェクトの決定
	std::vector<PostEffect> effectsToApply = activeEffects_;
	if (effectsToApply.empty()) {
		effectsToApply.push_back(PostEffect::kFullScreen);
	}

	size_t numEffects = effectsToApply.size();

	// 現在の入力テクスチャと中間出力先テクスチャのポインタ
	RenderTexture* currentInput = renderTexture_.get();
	RenderTexture* nextTempOutput = tempRenderTexture_.get();

	for (size_t i = 0; i < numEffects; ++i) {
		PostEffect effect = effectsToApply[i];
		bool isLast = (i == numEffects - 1);

		// 出力先の決定
		RenderTexture* outputTexture = nullptr;
		ID3D12Resource* outputResource = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};

		if (isLast) {
#ifdef USE_IMGUI
			outputTexture = postEffectTexture_.get();
			outputResource = outputTexture->GetResource();
			rtvHandle = outputTexture->GetRtvHandle();
#else
			outputResource = swapChainResources_[backBufferIndex].Get();
			rtvHandle = rtvHandles_[backBufferIndex];
#endif
		} else {
			outputTexture = nextTempOutput;
			outputResource = outputTexture->GetResource();
			rtvHandle = outputTexture->GetRtvHandle();
		}

		// 出力先を RENDER_TARGET に遷移
		D3D12_RESOURCE_BARRIER outBarrier{};
		outBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		outBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		outBarrier.Transition.pResource = outputResource;
		
		if (isLast) {
#ifdef USE_IMGUI
			outBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			outBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
#else
			outBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			outBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
#endif
		} else {
			if (i == 0) {
				outBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
			} else {
				outBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
			outBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		commandList_->ResourceBarrier(1, &outBarrier);

		// OMSetRenderTargets
		commandList_->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// クリア
		if (outputTexture) {
			outputTexture->ClearView(commandList_.Get());
		} else {
			float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
			commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		}

		// エフェクトに対応するPSOの選択
		PrimitiveType postEffectType = PrimitiveType::kPostEffectFullScreen;
		switch (effect) {
		case PostEffect::kFullScreen:
			postEffectType = PrimitiveType::kPostEffectFullScreen;
			break;
		case PostEffect::kBoxFilter:
			postEffectType = PrimitiveType::kPostEffectBoxFilter;
			break;
		case PostEffect::kGaussianFilter:
			postEffectType = PrimitiveType::kPostEffectGaussianFilter;
			break;
		case PostEffect::kLuminanceBasedOutline:
			postEffectType = PrimitiveType::kPostEffectLuminanceOutline;
			break;
		case PostEffect::kDepthBasedOutline:
			postEffectType = PrimitiveType::kPostEffectDepthOutline;
			break;
		case PostEffect::kRadialBlur:
			postEffectType = PrimitiveType::kPostEffectRadialBlur;
			break;
		case PostEffect::kDissolve:
			postEffectType = PrimitiveType::kPostEffectDissolve;
			break;
		case PostEffect::kRandomNoise:
			postEffectType = PrimitiveType::kPostEffectRandomNoise;
			break;
		case PostEffect::kHSVFilter:
			postEffectType = PrimitiveType::kPostEffectHSVFilter;
			break;
		default:
			break;
		}

		// 描画実行
		commandList_->SetGraphicsRootSignature(pso->GetRootSignature(postEffectType));
		commandList_->SetPipelineState(pso->GetPipelineState(
			device_.Get(), postEffectType, BlendMode::kNone, D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE
		));
		commandList_->SetGraphicsRootDescriptorTable(0, SrvManager::GetInstance()->GetGPUHandle(currentInput->GetSrvIndex()));
		if (effect == PostEffect::kDissolve && dissolveMaskTextureHandle_ >= 0) {
			commandList_->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetGPUHandle(dissolveMaskTextureHandle_));
		} else {
			commandList_->SetGraphicsRootDescriptorTable(1, SrvManager::GetInstance()->GetGPUHandle(depthStencil_->GetSrvIndex()));
		}
		commandList_->SetGraphicsRootConstantBufferView(2, fullScreenCB_->GetGPUVirtualAddress());

		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->DrawInstanced(3, 1, 0, 0);

		// 描画後、出力先を PIXEL_SHADER_RESOURCE に遷移させる
		if (!isLast) {
			D3D12_RESOURCE_BARRIER postBarrier = outputTexture->CreateTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList_->ResourceBarrier(1, &postBarrier);

			// ピンポンポインタの更新
			nextTempOutput = currentInput;
			currentInput = outputTexture;
		} else {
#ifdef USE_IMGUI
			// ImGui使用時は、描画完了した postEffectTexture_ を ピクセルシェーダー用(SRV) に戻す
			D3D12_RESOURCE_BARRIER peBarrierEnd = postEffectTexture_->CreateTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			
			// 同時に、バックバッファを PRESENT から RENDER_TARGET に遷移させ、ImGuiUIの描画ターゲットにする
			D3D12_RESOURCE_BARRIER bbBarrierEnd{};
			bbBarrierEnd.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			bbBarrierEnd.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			bbBarrierEnd.Transition.pResource = swapChainResources_[backBufferIndex].Get();
			bbBarrierEnd.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			bbBarrierEnd.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

			D3D12_RESOURCE_BARRIER barriersEnd[] = { peBarrierEnd, bbBarrierEnd };
			commandList_->ResourceBarrier(_countof(barriersEnd), barriersEnd);

			// ImGui描画ターゲットとしてバックバッファを設定
			commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, nullptr);
			// バックバッファをクリア（ImGui UI の背景になる）
			float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
			commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);
#endif
		}
	}
}

void DirectXCommon::RemovePostEffect(PostEffect effect) {
	auto it = std::remove(activeEffects_.begin(), activeEffects_.end(), effect);
	activeEffects_.erase(it, activeEffects_.end());
}

void DirectXCommon::SetFullScreenGray(bool isGray) {
	fullScreenMaterial_.isGray = isGray;
}

void DirectXCommon::SetFullScreenVignette(bool isVignette) {
	fullScreenMaterial_.isVignette = isVignette;
}

void DirectXCommon::SetFullScreenVignetteColor(const Vector3& color) {
	fullScreenMaterial_.vignetteColor = color;
}

void DirectXCommon::SetFullScreenVignetteScale(float scale) {
	fullScreenMaterial_.vignetteScale = scale;
}

void DirectXCommon::SetFullScreenVignettePower(float power) {
	fullScreenMaterial_.vignettePower = power;
}

void DirectXCommon::SetRadialBlurCenter(const Vector2& center) {
	fullScreenMaterial_.radialBlurCenter = center;
}

void DirectXCommon::SetRadialBlurWidth(float blurWidth) {
	fullScreenMaterial_.radialBlurWidth = blurWidth;
}

void DirectXCommon::SetDissolveThreshold(float threshold) {
	fullScreenMaterial_.dissolveThreshold = std::clamp(threshold, 0.0f, 1.0f);
}

void DirectXCommon::SetDissolveEdgeColor(const Vector3& color) {
	fullScreenMaterial_.dissolveEdgeColor = {
		std::clamp(color.x, 0.0f, 1.0f),
		std::clamp(color.y, 0.0f, 1.0f),
		std::clamp(color.z, 0.0f, 1.0f)
	};
}

void DirectXCommon::SetDissolveEdgeWidth(float width) {
	fullScreenMaterial_.dissolveEdgeWidth = std::clamp(width, 0.0f, 0.2f);
}

void DirectXCommon::SetNoiseAlpha(float alpha) {
	fullScreenMaterial_.noiseAlpha = std::clamp(alpha, 0.0f, 1.0f);
}

void DirectXCommon::SetHSVHueShift(float hueShift) {
	fullScreenMaterial_.hsvHueShift = std::clamp(hueShift, -1.0f, 1.0f);
}

void DirectXCommon::SetHSVSaturationMultiplier(float satMult) {
	fullScreenMaterial_.hsvSaturationMultiplier = satMult < 0.0f ? 0.0f : satMult;
}

void DirectXCommon::SetHSVValueMultiplier(float valMult) {
	fullScreenMaterial_.hsvValueMultiplier = valMult < 0.0f ? 0.0f : valMult;
}

void DirectXCommon::EndFrame() 
{	
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER endBarriers[4]{};

	// PRESENTへの遷移
	endBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	endBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	endBarriers[0].Transition.pResource = swapChainResources_[backBufferIndex].Get();
	endBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	endBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	// RenderTextureをCOMMONへ
	endBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	endBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	endBarriers[1].Transition.pResource = renderTexture_->GetResource();
	endBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	endBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;

	// DepthStencilをDEPTH_WRITEへ
	endBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	endBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	endBarriers[2].Transition.pResource = depthStencil_->GetResource();
	endBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	endBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	// tempRenderTextureをCOMMONへ
	endBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	endBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	endBarriers[3].Transition.pResource = tempRenderTexture_->GetResource();
	endBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	endBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;

	commandList_->ResourceBarrier(4, endBarriers);

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
	rtvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 5, false);
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
