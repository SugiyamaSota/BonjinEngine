#include "TextSprite.h"
#include "function/function.h" // CreateBufferResource用
#include "SrvManager.h"
#include <cassert>
#include <memory>
#include <cmath>

namespace Bonjin {

    TextSprite::TextSprite()
    {
        viewMatrix_ = MakeIdentity4x4();
        projectionMatrix_ = MakeOrthographicMatrix(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 100.0f);
        viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

        anchor_ = { 0.0f, 0.0f, 0.0f };
        size_ = { 100.0f, 100.0f };
        scale_ = { 1.0f, 1.0f };
        rotate_ = { 0.0f, 0.0f };
        translate_ = { 0.0f, 0.0f };
        color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

        dxCommon_ = DirectXCommon::GetInstance();
        device_ = dxCommon_->GetDevice();

        pso_ = dxCommon_->GetPSO()->GetPipelineState(
            device_, PrimitiveType::kSprite, BlendMode::kNormal,
            D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE
        );
    }

    TextSprite::~TextSprite()
    {
        if (vertexResource_) vertexResource_->Unmap(0, nullptr);
        if (materialResource_) materialResource_->Unmap(0, nullptr);
        if (wvpResource_) wvpResource_->Unmap(0, nullptr);
        if (indexResource_) indexResource_->Unmap(0, nullptr);
    }

    void TextSprite::Initialize() {
        // 頂点バッファ
        vertexResource_ = CreateBufferResource(device_, sizeof(VertexData) * 4);
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
        vertexBufferView_.StrideInBytes = sizeof(VertexData);
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

        // インデックスバッファ
        indexResource_ = CreateBufferResource(device_, sizeof(uint32_t) * 6);
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

        indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
        indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;

        // マテリアル
        materialResource_ = CreateBufferResource(device_, sizeof(Material));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
        materialData_->enableLighting = false;
        materialData_->uvTransform = MakeIdentity4x4();

        // WVP
        wvpResource_ = CreateBufferResource(device_, sizeof(TransformationMatrix));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));

        RefreshVertexData();
    }

    void TextSprite::SetText(const std::wstring& text, int fontSize, COLORREF textColor) {
        CreateTextTexture(text, fontSize, textColor);
        RefreshVertexData();
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

    void TextSprite::Update() {
        RefreshVertexData();

        Vector3 scale = { scale_.x, scale_.y, 0.0f };
        Vector3 rotate = { rotate_.x, rotate_.y, 0.0f };
        Vector3 translate = { translate_.x, translate_.y, 0.0f };

        Matrix4x4 worldMatrix = MakeAffineMatrix(scale, rotate, translate);

        wvpData_->World = worldMatrix;
        wvpData_->WVP = Multiply(worldMatrix, viewProjectionMatrix_);
        materialData_->color = color_;
        materialData_->uvTransform = MakeIdentity4x4();
    }

    void TextSprite::RefreshVertexData() {
        float left = -size_.x * anchor_.x;
        float right = size_.x * (1.0f - anchor_.x);
        float top = -size_.y * anchor_.y;
        float bottom = size_.y * (1.0f - anchor_.y);

        vertexData_[0] = { {left, bottom, 0.0f, 1.0f}, {0.0f, 1.0f}, {0,0,-1} };
        vertexData_[1] = { {left, top, 0.0f, 1.0f},    {0.0f, 0.0f}, {0,0,-1} };
        vertexData_[2] = { {right, bottom, 0.0f, 1.0f}, {1.0f, 1.0f}, {0,0,-1} };
        vertexData_[3] = { {right, top, 0.0f, 1.0f},    {1.0f, 0.0f}, {0,0,-1} };
    }

    void TextSprite::Draw() {
        if (!hasSrv_) return;

        auto commandList = dxCommon_->GetCommandList();

        commandList->SetGraphicsRootSignature(dxCommon_->GetPSO()->GetRootSignature(PrimitiveType::kSprite));
        commandList->SetPipelineState(pso_);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->IASetIndexBuffer(&indexBufferView_);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, SrvManager::GetInstance()->GetGPUHandle(srvIndex_));

        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
    }
}
