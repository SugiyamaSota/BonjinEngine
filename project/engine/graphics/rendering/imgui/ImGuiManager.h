#pragma once
#include<Windows.h>

// DirectX関連のSDKヘッダー
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <wrl/client.h>

// ImGui関連の外部ライブラリヘッダー
#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#include "../../externals/imgui/imgui_impl_dx12.h"
#include "../../externals/imgui/imgui_impl_win32.h"
#endif

class ImGuiManager
{
public:
	// シングルトンインスタンスの取得
	static ImGuiManager* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 最初のフレーム
	/// </summary>
	void NewFrame();

	/// <summary>
	/// 最後のフレーム
	/// </summary>
	void EndFrame();

	static void DestroyInstance();

private:
	// コンストラクタとデストラクタをprivateにする
	ImGuiManager();
	~ImGuiManager();

	// コピーコンストラクタと代入演算子を禁止する
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

	// 静的メンバ変数の追加
	static ImGuiManager* sInstance;
};