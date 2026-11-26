#pragma once

#include "IGamePhase.h"
#include "PostEffectTypes.h"
#include "vector.h"
#include <memory>

namespace Bonjin {

class BattleController;

namespace GamePhase {

class IHighlightStep; // ハイライト演出の1段階 (State)。中身は Highlight.cpp

/// <summary>
/// ハイライト演出フェーズ（カメラパン、実体化演出など）
/// フェーズの中の段階 (フェードイン -> カメラパン -> フェードアウト ... -> プレイヤーへ戻る) も
/// それぞれ State として IHighlightStep のクラスに分けている
/// </summary>
class Highlight : public IGamePhase {
public:
	Highlight();
	~Highlight() override;

	void Enter(GameScene* scene) override;
	void Update(GameScene* scene, float deltaTime) override;
	void DrawImGui(GameScene* scene) override;
	void Exit(GameScene* scene) override;

	void FinishHighlight(GameScene* scene);

private:
	void SetupHighlightDissolve();
	// 段階を切り替える (経過時間を0に戻して、次の段階の Enter を呼ぶ)
	void ChangeStep(std::unique_ptr<IHighlightStep> next, BattleController& battleController);

	std::unique_ptr<IHighlightStep> step_;
	float timer_ = 0.0f; // 今の段階に入ってからの経過時間
	DissolveMode dissolveMode_ = DissolveMode::kRightToLeft;
};

}

}
