#pragma once

#include "IResultPhase.h"

namespace Bonjin::ResultPhase {

/// <summary>
/// タイトルへの遷移待ちフェーズ
/// </summary>
class FadeOut : public IResultPhase {
public:
	void Update(ResultScene* scene, float deltaTime) override;
};

}
