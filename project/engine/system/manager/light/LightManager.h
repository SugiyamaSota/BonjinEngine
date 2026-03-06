#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "math/Struct.h"

class LightManager {
public:
    static LightManager* GetInstance();

    void Initialize(ID3D12Device* device);
    void Update();
    void DrawImGui();

    void Finalize();

    // Getter
    ID3D12Resource* GetDirectionalLightResource() { return directionalLightResource_.Get(); }
    ID3D12Resource* GetPointLightResource() { return pointLightResource_.Get(); }
    ID3D12Resource* GetSpotLightResource() { return spotLightResource_.Get(); }

private:
    LightManager() = default;
    ~LightManager() = default;

    // ディレクショナル
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;

    // ポイント
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    PointLight* pointLightData_ = nullptr;

    // スポット
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    SpotLight* spotLightData_ = nullptr;

};