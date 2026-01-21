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
		float phaseTimer_ = 0.0f;

	public:
		virtual ~BaseScene() = default;

		// フェーズ変更
		virtual void ChangePhase(T nextPhase) {
			phase_ = nextPhase;
			phaseTimer_ = 0.0f;
		}

		void UpdatePhaseTimer(float deltaTime) {
			phaseTimer_ += 1.f * deltaTime;
		}

		T GetPhase() const { return phase_; }
		float GetPhaseTimer() const { return phaseTimer_; }
	};

}

// このクラスまだつくっただけ