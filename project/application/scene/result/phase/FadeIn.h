#pragma once

#include "IResultPhase.h"

namespace Bonjin::ResultPhase {

/// <summary>
/// 入場フェーズ（入場トランジション終了後に操作フェーズへ）
/// </summary>
class FadeIn : public IResultPhase {
public:
	void Update(ResultScene* scene, float deltaTime) override;
};

}
