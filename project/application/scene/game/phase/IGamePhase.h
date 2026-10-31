#pragma once

#include "../../engine/interface/BaseScene.h"

namespace Bonjin {

class GameScene;

/// <summary>
/// GameSceneのフェーズ基底インターフェース
/// </summary>
using IGamePhase = IScenePhase<GameScene>;

}
