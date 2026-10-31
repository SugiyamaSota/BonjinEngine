#pragma once

#include "../../engine/interface/BaseScene.h"

namespace Bonjin {

class TitleScene;

/// <summary>
/// TitleSceneのフェーズ基底インターフェース
/// </summary>
class ITitlePhase : public IScenePhase<TitleScene> {
public:
	virtual ~ITitlePhase() = default;

	// ダイブ演出中かどうか
	virtual bool IsDiving() const { return false; }

	// ダイブの進行度 (0.0f - 1.0f)
	virtual float GetDiveProgress() const { return 1.0f; }
};

}
