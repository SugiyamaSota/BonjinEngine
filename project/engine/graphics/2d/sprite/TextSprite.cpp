#include "TextSprite.h"
#include "function/function.h" // CreateBufferResource用
#include "SrvManager.h"
#include <cassert>
#include <memory>
#include <cmath>

namespace Bonjin {

    TextSprite::TextSprite() : BaseSprite()
    {
    }

    TextSprite::~TextSprite()
    {
    }

    void TextSprite::Initialize() {
        BaseSprite::Initialize();
    }

    void TextSprite::SetText(const std::wstring& text, int fontSize, COLORREF textColor) {
        CreateTextTexture(text, fontSize, textColor);
        RefreshVertexData();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE TextSprite::GetSRVHandle() const {
        if (!hasSrv_) {
            return SrvManager::GetInstance()->GetGPUHandle(0);
        }
        return SrvManager::GetInstance()->GetGPUHandle(srvIndex_);
    }

    void TextSprite::CreateTextTexture(const std::wstring& text, int fontSize, COLORREF textColor) {
        // 1. GDIを利用したテキストのビットマップレンダリング
        HDC hdc = CreateCompatibleDC(nullptr);
        assert(hdc != nullptr);

        HFONT hFont = CreateFontW(
            fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Meiryo"
        );
        HGDIOBJ oldFont = SelectObject(hdc, hFont);

        SIZE textSize = { 0, 0 };
        GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.length()), &textSize);

        uint32_t width = (textSize.cx + 3) & ~3;
        uint32_t height = (textSize.cy + 3) & ~3;
        if (width == 0) width = 4;
        if (height == 0) height = 4;

        texWidth_ = width;
        texHeight_ = height;
        size_ = { (float)width, (float)height };

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -static_cast<LONG>(height);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* pBits = nullptr;
        HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        HGDIOBJ oldBmp = SelectObject(hdc, hBmp);

        if (pBits != nullptr) {
            // 背景黒でクリア
            RECT rect = { 0, 0, (LONG)width, (LONG)height };
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);

            SetTextColor(hdc, RGB(255, 255, 255)); // 白で描画し輝度を取り出す
            SetBkMode(hdc, TRANSPARENT);
            DrawTextW(hdc, text.c_str(), -1, &rect, DT_LEFT | DT_TOP);

            // 滑らかなアルファ値マッピング処理
            uint32_t* pixels = static_cast<uint32_t*>(pBits);
            BYTE rTarget = GetRValue(textColor);
            BYTE gTarget = GetGValue(textColor);
            BYTE bTarget = GetBValue(textColor);

            for (uint32_t i = 0; i < width * height; ++i) {
                uint32_t color = pixels[i];
                BYTE b = color & 0xFF;
                BYTE g = (color >> 8) & 0xFF;
                BYTE r = (color >> 16) & 0xFF;

                BYTE alpha = static_cast<BYTE>((r + g + b) / 3);

                if (alpha > 0) {
                    // 背景黒から抽出した輝度をアルファとし、カラーは設定されたRGB値に設定
                    pixels[i] = (alpha << 24) | (bTarget << 16) | (gTarget << 8) | rTarget;
                } else {
                    pixels[i] = 0x00000000;
                }
            }
        }

        // 2. DirectX12 デフォルトヒープテクスチャの作成
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        Microsoft::WRL::ComPtr<ID3D12Resource> textTexture;
        HRESULT hr = device_->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(textTexture.GetAddressOf())
        );
        assert(SUCCEEDED(hr));

        // 3. 中間バッファを作成しGPUへコピー
        uint64_t uploadSize = width * height * 4;
        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = CreateBufferResource(device_, uploadSize);
        
        if (pBits != nullptr) {
            void* pData = nullptr;
            uploadBuffer->Map(0, nullptr, &pData);
            memcpy(pData, pBits, uploadSize);
            uploadBuffer->Unmap(0, nullptr);
        }

        // オブジェクトの選択を復元（これをしないと正しく削除されずリークする）
        SelectObject(hdc, oldFont);
        SelectObject(hdc, oldBmp);

        // GDIオブジェクト解放
        DeleteObject(hFont);
        DeleteObject(hBmp);
        DeleteDC(hdc);

        // コピー用一時コマンドリストの作成と実行
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> tempAllocator;
        device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(tempAllocator.GetAddressOf()));
        
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> tempCmdList;
        device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, tempAllocator.Get(), nullptr, IID_PPV_ARGS(tempCmdList.GetAddressOf()));

        D3D12_TEXTURE_COPY_LOCATION dst{};
        dst.pResource = textTexture.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION src{};
        src.pResource = uploadBuffer.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Offset = 0;
        src.PlacedFootprint.Footprint.Width = width;
        src.PlacedFootprint.Footprint.Height = height;
        src.PlacedFootprint.Footprint.Depth = 1;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Footprint.RowPitch = width * 4;

        tempCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        // バリアの変更 (COPY_DEST -> GENERIC_READ)
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = textTexture.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        tempCmdList->ResourceBarrier(1, &barrier);

        tempCmdList->Close();
        ID3D12CommandList* lists[] = { tempCmdList.Get() };
        dxCommon_->GetCommandQueue()->ExecuteCommandLists(1, lists);

        // 同期待機
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        dxCommon_->GetCommandQueue()->Signal(fence.Get(), 1);
        if (fence->GetCompletedValue() < 1) {
            fence->SetEventOnCompletion(1, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
        }
        CloseHandle(eventHandle);

        // 古いテクスチャの解放とSRVの生成
        textureResource_ = textTexture;

        if (!hasSrv_) {
            srvIndex_ = SrvManager::GetInstance()->Allocate();
            hasSrv_ = true;
        }

        SrvManager::GetInstance()->CreateSrv(srvIndex_, textureResource_.Get(), SrvType::Texture2D);
    }
}
