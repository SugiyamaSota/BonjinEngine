#pragma once
#include "math/Struct.h"
#include "Camera.h"

namespace Bonjin {

class CameraController {
public:
	CameraController() = default;
	~CameraController() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="camera">制御対象のカメラへのポインタ</param>
	/// <param name="initialTarget">初期ターゲット座標</param>
	void Initialize(Camera* camera, const Vector3& initialTarget);

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="destinationTarget">目的地のターゲット座標</param>
	/// <param name="deltaTime">経過時間</param>
	void Update(const Vector3& destinationTarget, float deltaTime);

	/// <summary>
	/// ImGuiでの編集用描画
	/// </summary>
	void DrawImGui();

private:
	Camera* camera_ = nullptr;
	Vector3 currentTarget_ = { 0.0f, 0.0f, 0.0f };

	bool isLerpEnabled_ = true;
	float lerpFactor_ = 5.0f; // 追従速度係数 (大きいほど素早く追従)
};

}
