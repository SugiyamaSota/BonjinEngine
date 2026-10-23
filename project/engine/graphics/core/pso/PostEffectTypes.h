#pragma once
#include <cstdint>

/// <summary>
/// ポストエフェクトの種類（DirectXCommonから独立した型）
/// </summary>
enum class PostEffect {
	kFullScreen,
	kBoxFilter,
	kGaussianFilter,
	kLuminanceBasedOutline,
	kDepthBasedOutline,
	kRadialBlur,
	kDissolve,
	kRandomNoise,
	kHSVFilter,
	kCRTShutDown,
	kDigitalDissolve,
	kSimpleFade,
};

/// <summary>
/// デジタルディゾルブの進行方向/スキャンモード
/// </summary>
enum class DissolveMode : int32_t {
	kBottomToTop = 0, // 下から上（デフォルト）
	kTopToBottom = 1, // 上から下
	kRandom = 2,      // 画面全体からランダム
	kLeftToRight = 3, // 左から右
	kRightToLeft = 4, // 右から左
};
