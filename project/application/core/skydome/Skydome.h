#pragma once
#include "BonjinEngine.h"

class Skydome {
private:
	//ワールド変換
	WorldTransform worldTransform_;
	//モデル
	Model* model_ = nullptr;
	// カメラ
	Camera* camera_ = nullptr;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Model* model,Camera* camera);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();
};