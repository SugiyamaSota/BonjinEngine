#include "Camera.h"
#include"../bonjin/BonjinEngine.h"

const float PI = 3.1415926535f;

float DegToRad(float deg) {
	return deg * (PI / 180.0f);
}

void Camera::Initialize(uint32_t clientWidth, uint32_t clientHeight) {
	radius_ = 50.0f;
	direction_ = Direction::kFront;

	if (direction_ == Direction::kFront) {
		theta_ = DegToRad(180.0f);
	} else {
		theta_ = DegToRad(0.0f);
	}
	phi_ = DegToRad(75.0f);
	targetPosition_ = { 0,0,0 };
	isTargeting_ = true;
	farZ_ = 1000.0f;

	viewMatrix_ = MakeIdentity4x4();
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(clientWidth) / float(clientHeight), 0.1f, farZ_);
}

void Camera::Update(CameraType type) {
	if (Input::GetInstance()->IsTrigger(DIK_R)) {
		ResetPosition();
		ResetRotation();
	}
	switch (type) {
	case CameraType::kNormal:
		break;
	case CameraType::kDebug:
		Move();
		Rotate();
		break;
	}

	CalculateCameraPositionAndRotation();

	viewMatrix_ = MakeLookAtMatrix(translation_, targetPosition_, { 0, 1, 0 });

	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}

void Camera::Move() {
	Input* input = Input::GetInstance();

	// マウスホイールによるズーム
	if (input->GetMouseWheel() != 0) {
		long mouseDeltaZ = input->GetMouseWheel();

		// multiSpeed_ -> zoomSpeed_ に変更
		radius_ -= static_cast<float>(mouseDeltaZ) * zoomSpeed_;
		radius_ = std::clamp(radius_, 1.0f, 500.0f);
	}

	// マウスによる上下左右移動
	if (input->IsMousePress(2) && input->IsPress(DIK_LSHIFT)) {
		long mouseDeltaX = input->GetMouseDeltaX();
		long mouseDeltaY = input->GetMouseDeltaY();


		Vector3 moveLocal = {
			// multiSpeed_ -> moveSpeed_ に変更
			static_cast<float>(-mouseDeltaX) * moveSpeed_, // X軸方向 (左右)
			// multiSpeed_ -> moveSpeed_ に変更
			static_cast<float>(mouseDeltaY) * moveSpeed_,  // Y軸方向 (上下)
			0.0f
		};


		// ローカル移動ベクトルをワールド座標系に変換
		Matrix4x4 cameraWorldMatrix = Inverse(viewMatrix_);
		Vector3 worldRight = { cameraWorldMatrix.m[0][0], cameraWorldMatrix.m[0][1], cameraWorldMatrix.m[0][2] };
		Vector3 worldUp = { cameraWorldMatrix.m[1][0], cameraWorldMatrix.m[1][1], cameraWorldMatrix.m[1][2] };

		// worldRightとworldUpを使ってtargetPosition_を更新
		targetPosition_ = Add(targetPosition_, Multiply(moveLocal.x, worldRight));
		targetPosition_ = Add(targetPosition_, Multiply(moveLocal.y, worldUp));
	}
}

void Camera::Rotate() {
	Input* input = Input::GetInstance();

	// マウスによる回転処理
	if (input->IsMousePress(2) && !input->IsPress(DIK_LSHIFT)) {
		long mouseDeltaX = input->GetMouseDeltaX();
		long mouseDeltaY = input->GetMouseDeltaY();

		// Y軸回転 (multiSpeed_ -> rotateSpeed_ に変更)
		theta_ += static_cast<float>(mouseDeltaX) * rotateSpeed_;

		// X軸回転 (multiSpeed_ -> rotateSpeed_ に変更)
		phi_ += static_cast<float>(-mouseDeltaY) * rotateSpeed_;

		// 角度の最大最小
		const float MIN_PHI = DegToRad(1.0f);
		const float MAX_PHI = DegToRad(179.0f);
		phi_ = std::clamp(phi_, MIN_PHI, MAX_PHI);

	}
}
void Camera::CalculateCameraPositionAndRotation() {
	// XZ平面での投影された半径
	float projectedRadius = radius_ * std::sin(phi_);

	// カメラのワールド座標を計算
	translation_.x = targetPosition_.x + projectedRadius * std::sin(theta_);
	translation_.z = targetPosition_.z + projectedRadius * std::cos(theta_);
	translation_.y = targetPosition_.y + radius_ * std::cos(phi_);

	matRot_ = Multiply(MakeRotateYMatrix(theta_), MakeRotateXMatrix(phi_));
}

void Camera::ResetPosition() {
	radius_ = 50.0f;
	targetPosition_ = { 0,0,0 };
}

void Camera::ResetRotation() {
	theta_ = DegToRad(0.0f);
	phi_ = DegToRad(90.0f);
}

Matrix4x4 Camera::MakeLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up) {
	Vector3 zaxis = Normalize(Subtract(target, eye));
	//Vector3 xaxis = Normalize(Cross(zaxis, up));
	//Vector3 yaxis = Normalize(Cross(xaxis, zaxis));
	Vector3 xaxis = Normalize(Cross(up, zaxis));
	Vector3 yaxis = Normalize(Cross(zaxis, xaxis));

	Matrix4x4 result;
	result.m[0][0] = xaxis.x;
	result.m[0][1] = yaxis.x;
	result.m[0][2] = zaxis.x;
	result.m[0][3] = 0.0f;

	result.m[1][0] = xaxis.y;
	result.m[1][1] = yaxis.y;
	result.m[1][2] = zaxis.y;
	result.m[1][3] = 0.0f;

	result.m[2][0] = xaxis.z;
	result.m[2][1] = yaxis.z;
	result.m[2][2] = zaxis.z;
	result.m[2][3] = 0.0f;

	result.m[3][0] = -Dot(xaxis, eye);
	result.m[3][1] = -Dot(yaxis, eye);
	result.m[3][2] = -Dot(zaxis, eye);
	result.m[3][3] = 1.0f;

	return result;
}

void Camera::SetTarget(Vector3 targetPosition) {
	targetPosition_ = targetPosition; isTargeting_ = true;
}

Vector3 Camera::Project(const Vector3& worldPos) {

	float clientWidth = float(WinApp::GetInstance()->GetClientWidth());
	float clientHeight = float(WinApp::GetInstance()->GetClientHeight());

	// 2. ビュープロジェクション行列を使って、ワールド座標をスクリーン空間のニア・ファー平面（-1.0 ～ 1.0）へ変換
	// 内部で w 除算が行われる既存の Conversion 関数を利用します
	Vector3 ndcPos = Conversion(worldPos, viewProjectionMatrix_);

	// 3. ビューポート変換を行ってスクリーン座標（ピクセル単位）へ変換
	// ビューポート行列と同じ計算を適用します
	Vector3 screenPos;
	screenPos.x = (ndcPos.x + 1.0f) * clientWidth * 0.5f;
	screenPos.y = (1.0f - ndcPos.y) * clientHeight * 0.5f; // Y軸は上下反転
	screenPos.z = ndcPos.z; // 深度値 (0.0 ～ 1.0)

	return screenPos;
}

Vector3 Camera::GetWorldPosition() const{
	// ビュー行列の逆行列（カメラのワールド行列）を計算
	Matrix4x4 worldMatrix = Inverse(viewMatrix_);

	// 4行目の成分がワールド座標
	return { worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2] };
}