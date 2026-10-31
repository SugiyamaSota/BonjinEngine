#pragma once

#include "IResultPhase.h"

namespace Bonjin::ResultPhase {

/// <summary>
/// 入場フェーズ（1フレーム待ってから操作フェーズへ）
/// </summary>
class FadeIn : public IResultPhase {
public:
	void Update(ResultScene* scene, float deltaTime) override;
};

}
