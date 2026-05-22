#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <dxcapi.h>

// シェーダーをコンパイルするクラス
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    void InitializeDxc();

    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
        const std::wstring& filePath,
        const wchar_t* profile
    );

private:
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
};