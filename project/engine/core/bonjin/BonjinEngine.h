#pragma once
#include"../audio/AudioPlayer.h"
#include"../camera/Camera.h"
#include"DirectXCommon.h"
#include"function/function.h"
#include"input/Input.h"
#include"math/Struct.h"
#include"math/Vector.h"
#include"math/Matrix.h"
#include"math/Convert.h"
#include"Model.h"
#include"sprite/sprite.h"
#include"color/Color.h"
#include"windows/WinApp.h"
#include"time/Time.h"
#include"Particle.h"
#include"pso/PSOManager.h"

#include"ImGuiManager.h"
#include"TextureManager.h"
#include"ModelManager.h"
#include"LightManager.h"

/// <summary>
/// リソースリークチェッカーの構造体
/// </summary>
struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(debug.GetAddressOf())))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

namespace Bonjin {
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了関数
	/// </summary>
	void Finalize();

	/// <summary>
	/// 最初のフレームに呼び出し
	/// </summary>
	void NewFrame();

	/// <summary>
	/// 描画前に呼び出し
	/// </summary>
	void PreDraw();

	/// <summary>
	/// 最後のフレームに呼び出し
	/// </summary>
	void EndFrame();
}

