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

    static void DestroyInstance();

    // Getter
    ID3D12Resource* GetDirectionalLightResource() { return directionalLightResource_.Get(); }
    ID3D12Resource* GetPointLightResource() { return pointLightResource_.Get(); }

private:
    LightManager() = default;
    ~LightManager() = default;

    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    PointLight* pointLightData_ = nullptr;

    static LightManager* instance_;
};