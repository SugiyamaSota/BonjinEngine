#pragma once

#include "../../engine/interface/BaseScene.h"

namespace Bonjin {

class TutorialScene;

/// <summary>
/// TutorialSceneのフェーズ基底インターフェース
/// </summary>
using ITutorialPhase = IScenePhase<TutorialScene>;

}
