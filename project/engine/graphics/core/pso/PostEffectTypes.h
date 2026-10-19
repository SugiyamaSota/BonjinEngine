#pragma once

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
