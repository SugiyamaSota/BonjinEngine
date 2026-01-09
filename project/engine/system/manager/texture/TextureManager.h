#pragma once
#include"BonjinEngine.h"

#include <d3d12.h>
#include <vector>
#include <string>
#include <wrl/client.h>
#include<map>
#include <cassert>

#include "../../externals/DirectXTex/d3dx12.h"
#include "../../externals/DirectXTex/DirectXTex.h"

class TextureManager {
public:
    /// --- インスタンス関連 ---
    /// <summary>
    /// インスタンスを生成
    /// </summary>
    static TextureManager* GetInstance();

    /// <summary>
    /// インスタンスを破棄
    /// </summary>
    static void DestroyInstance();

    /// <summary>
    /// コピーコンストラクタを削除
    /// </summary>
    TextureManager(const TextureManager&) = delete;

    /// <summary>
    /// コピー演算子を削除
    /// </summary>
    TextureManager& operator=(const TextureManager&) = delete;

    /// --- 汎用関数 ---
    /// <summary>
    /// 初期化処理
    /// </summary>
    void Initialize();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~TextureManager();

    /// <summary>
    /// テクスチャを読み込む
    /// </summary>
    /// <param name="filePath">テクスチャ名</param>
    /// <returns>インデックス</returns>
    int LoadTexture(const std::string& filePath);

    /// <summary>
    /// GPUディスクリプタハンドルを取得
    /// </summary>
    /// <param name="textureIndex">取得したいインデックス</param>
    /// <returns>ディスクリプタハンドル</returns>
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int textureIndex) const;

    /// <summary>
    /// メタデータを取得
    /// </summary>
    /// <param name="textureIndex">取得したいインデックス</param>
    /// <returns>メタデータ</returns>
    const DirectX::TexMetadata& GetMetadata(int textureIndex) const;

    /// <summary>
    /// GPUを待機する処理
    /// </summary>
    void ReleaseIntermediateResources();

private:
    /// --- 変数 ---
    // インスタンス
    static TextureManager* instance_;

    // ロードされたテクスチャリソース
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textureResources_;

    // GPUディスクリプタハンドル
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> gpuDescriptorHandles_;

    // CPUディスクリプタハンドル
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuDescriptorHandles_;

    // テクスチャのメタデータ
    std::vector<DirectX::TexMetadata> metadatas_;

    // マネージャー内部のインデックス
    uint32_t nextHandleIndex_ = 0;

    // デスクリプタヒープ上の開始オフセット
    uint32_t srvHeapBaseOffset_ = 0;

    // テクスチャのファイル名
    std::map<std::string, int> loadedTextures_;

    // リソースの生存期間管理
    struct UploadResourceEntry {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        UINT64 fenceValue;
    };
    std::vector<UploadResourceEntry> pendingUploadResources_;

    /// --- 関数 ---
    /// <summary>
    /// 空のコンストラクタ
    /// </summary>
    TextureManager();

    /// <summary>
    /// 内部的なテクスチャ読み込み処理
    /// </summary
    DirectX::ScratchImage LoadTextureInternal(const std::string& filePath);
    
    /// <summary>
    /// 内部的なテクスチャリソース作成処理
    /// </summary>
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResourceInternal(ID3D12Device* device, const DirectX::TexMetadata& metadata);
    /// <summary>
    /// 内部的なテクスチャデータアップロード処理 (中間リソースを返す)
    /// </summary>
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureDataInternal(
        ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
};