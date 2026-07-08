#pragma once
#include "../bonjin/BonjinEngine.h"

#include <string>
#include <memory>
#include <vector>

class BaseObject;

namespace Bonjin
{
	// シーンの種類を識別するための型エイリアス
	using SceneType = std::string;

	class IScene 
	{
	protected:

	public:
		/// <summary>
		/// 仮想デストラクタ
		/// </summary>
		virtual ~IScene();

		// オブジェクトの登録と取得
		void RegisterObject(BaseObject* obj);
		void UnregisterObject(BaseObject* obj);
		const std::vector<BaseObject*>& GetSceneObjects() const { return sceneObjects_; }

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="camera">カメラ</param>
		virtual void Initialize(Camera* camera);

		/// <summary>
		/// 明示的にリソースの解放を行う
		/// </summary>
		virtual void Unload() = 0;

		/// <summary>
		/// 更新
		/// </summary>
		/// <param name="deltaTime">デルタタイム</param>
		virtual void Update(float deltaTime)=0;

		/// <summary>
		/// 描画
		/// </summary>
		virtual void Draw() = 0;

		/// <summary>
		/// 現在のシーンを取得
		/// </summary>
		SceneType GetCurrentSceneType() const { return currentSceneType_; }

		/// <summary>
		/// 次に遷移するシーン
		/// </summary>
		virtual SceneType GetNextScene() const = 0;

		/// <summary>
		/// シーンごとのImGui処理
		/// </summary>
		virtual void DrawSceneImGui() = 0;

		/// <summary>
		/// ImGui処理
		/// </summary>
		virtual void DrawImGui();

		/// <summary>
		/// シーン名の登録兼取得
		/// </summary>
		/// <returns></returns>
		virtual const char* GetScenename()const = 0;

	protected:
		// カメラ
		Camera* camera_ = nullptr;

		// 派生クラスでのみ書き換え可能な現在のシーンタイプ
		SceneType currentSceneType_ = "TitleScene";

		// 次に遷移したいシーンタイプ
		SceneType nextSceneType_ = "TitleScene";

		// シーン内の登録オブジェクト一覧
		std::vector<BaseObject*> sceneObjects_;

	};
}