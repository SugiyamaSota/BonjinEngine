#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include<vector>
#include<chrono>

#include"pso/PSOManager.h"
#include"math/Struct.h"

class DirectXCommon {
public:
	/// --- インスタンス関連 ---
	// 生成、取得
	static DirectXCommon* GetInstance();

	// 破棄
	static void DestroyInstance();

	// コピー禁止
	DirectXCommon(const DirectXCommon&) = delete;
	DirectXCommon& operator=(const DirectXCommon&) = delete;

	/// --- 汎用関数 ---
	// デストラクタ
	~DirectXCommon();

	// 初期化処
	void Initialize();

	// フレーム開始
	void NewFeame();

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

	// 深度
	ID3D12DescriptorHeap* GetDSVDescriptorHeap() { return dsvDescriptorHeap_.Get(); }

	// 光
	ID3D12Resource* GetDirectionalLightResource() { return directionalLightResource_.Get(); }

	// PSO
	PSOManager* GetPSO()const { return pso; }


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
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_ = nullptr;

	// ディスクリプタサイズ
	uint32_t descriptorSizeSRV_;
	uint32_t descriptorSizeRTV_;
	uint32_t descriptorSizeDSV_;

	// フェンス関連
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_;

	// 深度
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc_ = {};
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc_ = {};

	// PSO
	PSOManager* pso = nullptr;

	// 光
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;
	DirectionalLight* directionalLightData_ = nullptr;

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
	void CreateDepth();     // 深度
	void CreateLight();     // 光
	void InitializeFixFPS();// FPS固定初期化
	void UpdateFixFPS();    // FPS固定更新

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);
};