#pragma once

#include "IScene.h"
#include <memory>
#include <utility>

namespace Bonjin 
{
	/// <summary>
	/// シーンフェーズ（ステート）の基底インターフェース
	/// </summary>
	/// <typeparam name="TScene">対象のシーン型</typeparam>
	template <typename TScene>
	class IScenePhase
	{
	public:
		virtual ~IScenePhase() = default;

		/// <summary>
		/// フェーズ突入時の処理
		/// </summary>
		/// <param name="scene">シーンへのポインタ</param>
		virtual void Enter(TScene* scene) { (void)scene; }

		/// <summary>
		/// 毎フレームの更新処理
		/// </summary>
		/// <param name="scene">シーンへのポインタ</param>
		/// <param name="deltaTime">デルタタイム</param>
		virtual void Update(TScene* scene, float deltaTime) = 0;

		/// <summary>
		/// 描画処理（ステート固有の描画が必要な場合）
		/// </summary>
		/// <param name="scene">シーンへのポインタ</param>
		virtual void Draw(TScene* scene) { (void)scene; }

		/// <summary>
		/// ImGui描画処理（ステート固有のデバッグUI）
		/// </summary>
		/// <param name="scene">シーンへのポインタ</param>
		virtual void DrawImGui(TScene* scene) { (void)scene; }

		/// <summary>
		/// フェーズ離脱時の後処理
		/// </summary>
		/// <param name="scene">シーンへのポインタ</param>
		virtual void Exit(TScene* scene) { (void)scene; }
	};

	/// <summary>
	/// ステートパターンでフェーズを管理するシーン基底クラス
	/// </summary>
	/// <typeparam name="TScene">具象シーンクラス</typeparam>
	template <typename TScene>
	class BaseScene : public IScene
	{
	protected:
		// 現在のフェーズ
		std::unique_ptr<IScenePhase<TScene>> currentPhase_ = nullptr;

	public:
		virtual ~BaseScene() = default;

		/// <summary>
		/// フェーズを変更する（GoF Stateパターン）
		/// </summary>
		/// <param name="nextPhase">次のフェーズオブジェクト（std::make_uniqueで生成）</param>
		virtual void ChangePhase(std::unique_ptr<IScenePhase<TScene>> nextPhase) {
			if (currentPhase_) {
				currentPhase_->Exit(static_cast<TScene*>(this));
			}
			currentPhase_ = std::move(nextPhase);
			if (currentPhase_) {
				currentPhase_->Enter(static_cast<TScene*>(this));
			}
		}

		/// <summary>
		/// 現在のフェーズを取得
		/// </summary>
		IScenePhase<TScene>* GetCurrentPhase() const { return currentPhase_.get(); }
	};

	/// <summary>
	/// 既存の enum ベースの管理を行うシーン用（段階的移行の互換用）
	/// </summary>
	/// <typeparam name="T">フェーズのenum型</typeparam>
	template <typename T>
	class LegacyBaseScene : public IScene
	{
	protected:
		T phase_;

	public:
		virtual ~LegacyBaseScene() = default;

		virtual void ChangePhase(T nextPhase) {
			phase_ = nextPhase;
		}

		T GetPhase() const { return phase_; }
	};

}