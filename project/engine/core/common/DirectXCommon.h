#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include<vector>
#include<chrono>

#include"PSOManager.h"
#include"math/Struct.h"

#include "rendetTexture/RenderTexture.h"
#include "depthStencil/DepthStencil.h"

class DirectXCommon {
public:
	enum class PostEffect {
		kFullScreen,
		kBoxFilter,
		kGaussianFilter,
		kLuminanceBasedOutline,
		kDepthBasedOutline,
		kRadialBlur,
		kDissolve,
		kRandomNoise
	};

	/// --- インスタンス関連 ---
	// 生成、取得
	static DirectXCommon* GetInstance();

	// 破棄
	static void DestroyInstance();

	// コピー禁止
	DirectXCommon(const DirectXCommon&) = delete;
	DirectXCommon& operator=(const DirectXCommon&) = delete;

	void SetPostEffect(PostEffect effect) { currentEffect_ = effect; }
	PostEffect GetPostEffect() const { return currentEffect_; }
	void SetFullScreenGray(bool isGray);
	bool IsFullScreenGray() const { return fullScreenMaterial_.isGray != 0; }
	void SetFullScreenVignette(bool isVignette);
	bool IsFullScreenVignette() const { return fullScreenMaterial_.isVignette != 0; }
	void SetRadialBlurCenter(const Vector2& center);
	Vector2 GetRadialBlurCenter() const { return fullScreenMaterial_.radialBlurCenter; }
	void SetRadialBlurWidth(float blurWidth);
	float GetRadialBlurWidth() const { return fullScreenMaterial_.radialBlurWidth; }
	void SetDissolveThreshold(float threshold);
	float GetDissolveThreshold() const { return fullScreenMaterial_.dissolveThreshold; }
	void SetDissolveEdgeColor(const Vector3& color);
	Vector3 GetDissolveEdgeColor() const { return fullScreenMaterial_.dissolveEdgeColor; }
	void SetDissolveEdgeWidth(float width);
	float GetDissolveEdgeWidth() const { return fullScreenMaterial_.dissolveEdgeWidth; }
	void SetNoiseAlpha(float alpha);
	float GetNoiseAlpha() const { return fullScreenMaterial_.noiseAlpha; }

	/// --- 汎用関数 ---
	// デストラクタ
	~DirectXCommon();

	// 初期化処
	void Initialize();

	// フレーム開始
	void PreDraw();

	void PostDraw();

	// フレーム最後
	void EndFrame();

	// コマンドリストの実行と完了待機、リセット
	void WaitAndResetCommandList();

	/// --- 取得関数 ---
	// デバイス関連
	ID3D12Device* GetDevice() { return device_.Get(); }
	IDXGIFactory7* GetDxgiFactory() { return dxgiFactory_.Get(); }

	// コマンド関連
	ID3D12CommandQueue* GetCommandQueue() { return commandQueue_.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() { return commandAllocator_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList_.Get(); }

	// スワップチェーン関連
	IDXGISwapChain4* GetSwapChain() { return swapChain_.Get(); }
	DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc() { return swapChainDesc_; }
	ID3D12Resource* GetSwapChainResource(int32_t index) { return swapChainResources_[index].Get(); }

	// ディスクリプタヒープ
	ID3D12DescriptorHeap* GetRTVDescriptorHeap() { return rtvDescriptorHeap_.Get(); }
	D3D12_RENDER_TARGET_VIEW_DESC GetRTVDesc() { return rtvDesc_; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTVHandle(uint32_t index) { return rtvHandles_[index]; }
	ID3D12DescriptorHeap* GetSRVDescriptorHeap() { return srvDescriptorHeap_.Get(); }

	// ディスクリプタサイズ
	uint32_t GetRTVSize() { return descriptorSizeRTV_; }
	uint32_t GetSRVSize() { return descriptorSizeSRV_; }
	uint32_t GetDSVSize() { return descriptorSizeDSV_; }

	// フェンス関連
	ID3D12Fence* GetFence() { return fence_.Get(); }
	uint64_t GetFenceValue() { return fenceValue_; }
	void IncrementFencevalue() { fenceValue_++; }
	HANDLE GetFenceEvent() { return fenceEvent_; }

	uint64_t GetNextFenceValue() const { return fenceValue_ + 1; }

	// 深度
	ID3D12DescriptorHeap* GetDSVDescriptorHeap() { return depthStencil_->GetDescriptorHeap(); }

	// PSO
	PSOManager* GetPSO()const { return pso.get(); }

	// RenderTexture
	RenderTexture* GetRenderTexture() const { return renderTexture_.get(); }

private:
	// コンストラクタ
	DirectXCommon();

	// シングルトンインスタンス
	static DirectXCommon* instance_;

	/// --- 変数 ---
	// デバイス関連
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;

	// コマンド関連
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

	// スワップチェーン関連
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };

	// ディスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[3];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_ = nullptr;

	// ディスクリプタサイズ
	uint32_t descriptorSizeSRV_;
	uint32_t descriptorSizeRTV_;
	uint32_t descriptorSizeDSV_;

	// フェンス関連
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_;

	// PSO
	std::unique_ptr<PSOManager> pso = nullptr;

	// ビューポート
	D3D12_VIEWPORT viewport_{};

	// シザー矩形
	D3D12_RECT scissorRect_{};

	// バリア
	D3D12_RESOURCE_BARRIER barrier_{};

	// 記録時間
	std::chrono::steady_clock::time_point reference_;

	/// --- 関数 ---
	void CreateDevice();    // デバイス
	void CreateCommand();   // コマンド関連
	void CreateSwapChain(); // スワップチェーン
	void CreateFence();     // フェンス
	void InitializeFixFPS();// FPS固定初期化
	void UpdateFixFPS();    // FPS固定更新

	std::unique_ptr<RenderTexture> renderTexture_ = nullptr;

	std::unique_ptr<DepthStencil> depthStencil_ = nullptr;

	struct FullScreenMaterial {
		Matrix4x4 projectionInverse;
		int32_t isGray;
		int32_t isVignette;
		float padding[2];
		Vector2 radialBlurCenter;
		float radialBlurWidth;
		float dissolveThreshold;
		Vector3 dissolveEdgeColor;
		float dissolveEdgeWidth;
		float time;
		float noiseAlpha;
		float padding3[2];
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> fullScreenCB_ = nullptr;
	FullScreenMaterial* fullScreenData_ = nullptr;
	FullScreenMaterial fullScreenMaterial_{};
	int dissolveMaskTextureHandle_ = -1;

	PostEffect currentEffect_ = PostEffect::kFullScreen;

};
