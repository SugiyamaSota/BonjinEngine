#pragma once

#include"IScene.h"

namespace Bonjin 
{
	// TはTitlePgaseやGamePhaseなど
	template<typename T>
	class BaseScene : public IScene
	{
	protected:
		// 各シーン固有フェーズ
		T phase_;

	public:
		virtual ~BaseScene() = default;

		// フェーズ変更
		virtual void ChangePhase(T nextPhase) {
			phase_ = nextPhase;
		}

		// フェーズ取得
		T GetPhase() const { return phase_; }
	};

}

// このクラスまだつくっただけ