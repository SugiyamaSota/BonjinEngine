#pragma once
#include <cmath>
#include <algorithm>

#include"math/Struct.h"


class Camera {

	enum class Direction{
		kFront,
		kBack,
	};

private:
	///// --- 変数 --- /////
	// 累積回転行列
	Matrix4x4 matRot_;

	// ローカル座標
	Vector3 translation_;

	const Vector3 scale_{ 1,1,1 };

	// ビュー行列
	Matrix4x4 viewMatrix_;
	// 射影行列
	Matrix4x4 projectionMatrix_;
	// ビュープロジェクション行列
	Matrix4x4 viewProjectionMatrix_;

	// カメラの速度(ズーム、移動、回転)
	const float zoomSpeed_ = 0.01f;
	const float moveSpeed_ = 0.01f;
	const float rotateSpeed_ = 0.0005f;

	// ターゲット関連
	bool isTargeting_;       // フラグ
	Vector3 targetPosition_; // 注視する対象座標 (球面座標の中心)

	// 球面座標系に使う変数
	float radius_;
	float theta_;
	float phi_;

	// 遠クリップ距離 
	float farZ_;


	///// --- 関数 --- /////
	// 移動処理
	void Move();

	// 回転処理
	void Rotate();

	// 球面座標からカメラのワールド座標と回転行列を計算
	void CalculateCameraPositionAndRotation();

	// 対象を見つめる処理
	Matrix4x4 MakeLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up);

	// ターゲットするかどうかのチェック関数
	void Check() { isTargeting_ = false; }

	Direction direction_ = Direction::kFront;

	Vector3 shakeOffset_ = { 0.0f, 0.0f, 0.0f }; // シェイクによるズレ
	float shakeIntensity_ = 0.0f;              // 現在の強さ
	float shakeDuration_ = 0.0f;
public:
	enum class CameraType {
		kNormal,
		kDebug,
	};
	///// --- 関数 --- /////
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(uint32_t clientWidth, uint32_t clientHeight);

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="isDebug">デバッグカメラフラグ</param>
	void Update(CameraType type);

	/// <summary>
	/// 注視する対象の設定
	/// </summary>
	/// <param name="targetPosition">注視する座標</param>
	void SetTarget(Vector3 targetPosition);

	Vector3 Project(const Vector3& worldPos);

	///// --- ゲッター関数 --- /////
	Matrix4x4 GetViewMatrix()const { return viewMatrix_; }
	Matrix4x4 GetProjectionMatrix()const {return projectionMatrix_; }
	Matrix4x4 GetViewProjectionMatrix()const { return viewProjectionMatrix_; }
	Vector3 GetEye() const { return translation_; } // カメラの視点位置を返す
	float GetFarZ() const { return farZ_; }         // 遠クリップ距離を返す
	Vector3 GetWorldPosition()const ;

	///// --- セッター関数 --- /////
	void ResetPosition();
	void ResetRotation();
	void SetPosition(Vector3 position) { translation_ = position; }
	void StartShake(float intensity, float duration) {
		shakeIntensity_ = intensity;
		shakeDuration_ = duration;
	}
};