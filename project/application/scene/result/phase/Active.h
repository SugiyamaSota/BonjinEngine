#pragma once

#include "IResultPhase.h"

namespace Bonjin::ResultPhase {

/// <summary>
/// 入力待ちフェーズ（決定でタイトルへ）
/// </summary>
class Active : public IResultPhase {
public:
	void Update(ResultScene* scene, float deltaTime) override;
};

}
