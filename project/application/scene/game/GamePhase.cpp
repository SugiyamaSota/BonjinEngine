#include "GamePhase.h"
#include "GameScene.h"

#include "../result/ResultScene.h"
#include "SceneManager.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "vector.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace Bonjin {

// ===================================================================
// GamePhaseHighlight
// ===================================================================

void GamePhaseHighlight::SetupHighlightDissolve() {
	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->AddPostEffect(PostEffect::kDigitalDissolve);
	sm->SetDissolveMode(dissolveMode_);
	sm->SetDissolveThreshold(0.0f); // 真っ黒から実体化スタート
	sm->SetDissolveEdgeWidth(0.06f); // サイバー発光エッジ
	sm->SetDissolveEdgeColor({ 0.0f, 1.5f, 2.0f }); // サイバーブルー
}

void GamePhaseHighlight::Enter(GameScene* scene) {
	subState_ = SubState::kFadeIn;
	currentHighlightIndex_ = 0;
	timer_ = 0.0f;

	auto bc = scene->GetBattleController();
	if (!bc) {
		return;
	}

	// 演出中はHUDを非表示
	bc->SetHudVisible(false);

	const auto& highlights = bc->GetMapChipField()->GetHighlights();
	if (!highlights.empty()) {
		// 最初のハイライト位置へカメラを瞬時にワープ
		bc->GetCameraController()->SetTargetInstant(highlights[0].startWorldPos);
	}

	// 最初のカメラ位置に合わせて描画用モデルを初期更新
	bc->UpdateVisuals(0.0f);

	// 初回ハイライトのDigitalDissolveを開始
	SetupHighlightDissolve();
}

void GamePhaseHighlight::Update(GameScene* scene, float deltaTime) {
	auto sm = SceneManager::GetInstance();
	auto bc = scene->GetBattleController();
	if (!bc) {
		return;
	}

	// スキップ判定 (SPACEキー または GAMEPAD Aボタン)
	// ※ シーン切り替え直後の入力残りや連打による誤スキップを防ぐため0.2秒のガードを設ける
	if (timer_ > 0.2f && (Input::GetInstance()->IsTrigger(DIK_SPACE) || 
		Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A))) {
		FinishHighlight(scene);
		return;
	}

	const auto& highlights = bc->GetMapChipField()->GetHighlights();
	timer_ += deltaTime;

	switch (subState_) {
	case SubState::kFadeIn: {
		float t = std::clamp(timer_ / kFadeDuration, 0.0f, 1.0f);
		sm->SetFadeAmount(t); // 徐々に実体化（0.0 -> 1.0）

		if (timer_ >= kFadeDuration) {
			sm->ClearPostEffects(); // パン中は通常描画（エフェクトなしで鮮明に）
			sm->SetFadeAmount(1.0f);
			subState_ = SubState::kCameraPan;
			timer_ = 0.0f;
		}
		break;
	}
	case SubState::kCameraPan: {
		if (currentHighlightIndex_ < highlights.size()) {
			const auto& currentHighlight = highlights[currentHighlightIndex_];
			float duration = (std::max)(currentHighlight.displayDuration, 0.1f);
			float t = std::clamp(timer_ / duration, 0.0f, 1.0f);

			// スムーズな補間 (3t^2 - 2t^3)
			float smoothT = t * t * (3.0f - 2.0f * t);
			Vector3 targetPos = Add(currentHighlight.startWorldPos, Multiply(smoothT, currentHighlight.panDirection));
			bc->GetCameraController()->Update(targetPos, deltaTime);

			if (timer_ >= duration) {
				// 3つ目（または最後のハイライト）に達した場合はフェードアウトせずにカメラを引きながらプレイヤーへ移動
				bool isLastHighlight = (currentHighlightIndex_ >= 2 || currentHighlightIndex_ + 1 >= highlights.size());
				if (isLastHighlight) {
					subState_ = SubState::kReturnToPlayer;
					returnStartPos_ = targetPos;
					timer_ = 0.0f;
					// 画面は明るい状態（FadeAmount = 1.0f）を維持
					sm->ClearPostEffects();
					sm->SetFadeAmount(1.0f);
					sm->SetDissolveThreshold(1.0f);
				} else {
					// 次のハイライトへ向けて暗転フェードアウト開始
					sm->ClearPostEffects();
					sm->AddPostEffect(PostEffect::kSimpleFade);
					sm->SetFadeAmount(1.0f);
					subState_ = SubState::kFadeOut;
					timer_ = 0.0f;
				}
			}
		} else {
			subState_ = SubState::kReturnToPlayer;
			timer_ = 0.0f;
		}
		break;
	}
	case SubState::kReturnToPlayer: {
		float t = std::clamp(timer_ / kReturnDuration, 0.0f, 1.0f);

		// なめらかな補間カーブ (SmootherStep: 6t^5 - 15t^4 + 10t^3)
		float smoothT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

		Vector3 playerPos = bc->GetPlayer() ? bc->GetPlayer()->GetWorldPosition() : Vector3{ 0.0f, 0.0f, 0.0f };
		Vector3 currentTarget = Lerp(returnStartPos_, playerPos, smoothT);

		// カメラ注視点をプレイヤー位置へ向けて更新
		bc->GetCameraController()->SetTargetInstant(currentTarget);

		// 移動中にカメラを引き（ズームアウト）にし、プレイヤーに近づくにつれて通常距離に戻す
		float pullFactor = std::sin(t * 3.1415926535f);
		float currentRadius = Camera::kDefaultRadius + (kMaxPullBackRadius - Camera::kDefaultRadius) * pullFactor;
		bc->GetCameraController()->SetCameraRadius(currentRadius);

		if (timer_ >= kReturnDuration) {
			// プレイヤーに到着完了！カメラ距離を確実に通常に戻し、GOシグナルを発行
			bc->GetCameraController()->ResetCameraRadius();
			bc->GetCameraController()->SetTargetInstant(playerPos);
			FinishHighlight(scene);
			return;
		}
		break;
	}
	case SubState::kFadeOut: {
		float t = 1.0f - std::clamp(timer_ / kFadeDuration, 0.0f, 1.0f);
		sm->SetFadeAmount(t); // 徐々に暗くなる

		if (timer_ >= kFadeDuration) {
			currentHighlightIndex_++;
			timer_ = 0.0f;

			if (currentHighlightIndex_ < highlights.size()) {
				// 次のハイライトへカメラを瞬時に移動させて Fade In
				bc->GetCameraController()->SetTargetInstant(highlights[currentHighlightIndex_].startWorldPos);
				bc->UpdateVisuals(0.0f);

				subState_ = SubState::kFadeIn;
			} else {
				subState_ = SubState::kReturnToPlayer;
				if (bc->GetPlayer()) {
					returnStartPos_ = bc->GetPlayer()->GetWorldPosition();
				}
			}
		}
		break;
	}
	case SubState::kPlayerFocusIn: {
		// プレイヤー出現は DigitalDissolve（電子ブロック・実体化演出）
		float t = std::clamp(timer_ / kPlayerFocusDuration, 0.0f, 1.0f);
		sm->SetDissolveThreshold(t);

		if (bc->GetPlayer()) {
			bc->GetCameraController()->Update(bc->GetPlayer()->GetWorldPosition(), deltaTime);
		}

		if (timer_ >= kPlayerFocusDuration) {
			FinishHighlight(scene);
			return;
		}
		break;
	}
	default:
		break;
	}

	// 描画用モデル（カメラ追従WVP行列、天球、ブロック、ゴール、パーティクル等）を更新
	bc->UpdateVisuals(deltaTime);
}

void GamePhaseHighlight::DrawImGui(GameScene* scene) {
#ifdef USE_IMGUI
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "=== Stage Intro Highlight ===");
	auto bc = scene->GetBattleController();
	if (bc && bc->GetMapChipField()) {
		if (subState_ == SubState::kReturnToPlayer) {
			ImGui::Text("State: Return to Player (Pull-back Camera)");
		} else {
			ImGui::Text("Highlight: %zu / %zu", currentHighlightIndex_ + 1, bc->GetMapChipField()->GetHighlights().size());
		}
	}
	if (ImGui::Button("Skip Highlight")) {
		FinishHighlight(scene);
	}
	ImGui::Separator();
#else
	(void)scene;
#endif
}

void GamePhaseHighlight::Exit(GameScene* scene) {
	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->SetFadeAmount(1.0f);
	sm->SetDissolveThreshold(1.0f);

	auto bc = scene->GetBattleController();
	if (bc && bc->GetCameraController()) {
		bc->GetCameraController()->ResetCameraRadius();
	}
}

void GamePhaseHighlight::FinishHighlight(GameScene* scene) {
	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->SetFadeAmount(1.0f);
	sm->SetDissolveThreshold(1.0f);

	auto bc = scene->GetBattleController();
	if (bc) {
		if (bc->GetCameraController()) {
			bc->GetCameraController()->ResetCameraRadius();
			if (bc->GetPlayer()) {
				bc->GetCameraController()->SetTargetInstant(bc->GetPlayer()->GetWorldPosition());
			}
		}
		bc->SetHudVisible(true);
		bc->TriggerGoSignal();
	}

	// プレイフェーズへ遷移
	scene->ChangePhase(std::make_unique<GamePhasePlay>());
}

// ===================================================================
// GamePhasePlay
// ===================================================================

void GamePhasePlay::Enter(GameScene* scene) {
	auto bc = scene->GetBattleController();
	if (bc) {
		bc->SetHudVisible(true);
		if (!bc->IsGoSignalActive()) {
			bc->TriggerGoSignal();
		}
	}
}

void GamePhasePlay::Update(GameScene* scene, float deltaTime) {
	auto bc = scene->GetBattleController();
	if (!bc) {
		return;
	}

	bc->Update(deltaTime);

	if (bc->IsGoalReached()) {
		ResultScene::SetIsClear(true);
		scene->ChangePhase(std::make_unique<GamePhaseGoal>());
		SceneManager::GetInstance()->RequestSceneChange("ResultScene");
	} else if (bc->GetPlayer() && bc->GetPlayer()->GetStatus().IsDead()) {
		ResultScene::SetIsClear(false);
		scene->ChangePhase(std::make_unique<GamePhaseGameOver>());
	}
}

void GamePhasePlay::Exit(GameScene* scene) {
	(void)scene;
}

// ===================================================================
// GamePhaseGoal
// ===================================================================

void GamePhaseGoal::Enter(GameScene* scene) {
	(void)scene;
}

void GamePhaseGoal::Update(GameScene* scene, float deltaTime) {
	(void)scene;
	(void)deltaTime;
	// リザルトシーンへの遷移待ち
}

void GamePhaseGoal::Exit(GameScene* scene) {
	(void)scene;
}

// ===================================================================
// GamePhaseGameOver
// ===================================================================

void GamePhaseGameOver::BuildSpawnList() {
	spawnList_.clear();

	// セル1個あたりのサイズ
	const float cellW = kScreenW / kGridCols;
	const float cellH = kScreenH / kGridRows;

	std::uniform_real_distribution<float> distJX(-kJitter, kJitter);
	std::uniform_real_distribution<float> distJY(-kJitter, kJitter);
	// ウィンドウはセルより少し小さめ〜大きめでランダム（隙間が出ないようセル比で指定）
	std::uniform_real_distribution<float> distW(cellW * 1.25f, cellW * 1.5f);
	std::uniform_real_distribution<float> distH(cellH * 1.25f, cellH * 1.5f);

	for (int row = 0; row < kGridRows; ++row) {
		for (int col = 0; col < kGridCols; ++col) {
			SpawnCell cell;
			cell.x = col * cellW + distJX(rng_);
			cell.y = row * cellH + distJY(rng_);
			cell.w = distW(rng_);
			cell.h = distH(rng_);
			spawnList_.push_back(cell);
		}
	}

	// セルの出現順をシャッフル → ランダムに画面が埋まっていく
	std::shuffle(spawnList_.begin(), spawnList_.end(), rng_);
}

void GamePhaseGameOver::Enter(GameScene* scene) {
	timer_          = 0.0f;
	nextSpawnTimer_ = 0.0f;
	spawnCount_     = 0;
	sceneChangeRequested_ = false;
	windows_.clear();

	// 乱数初期化
	std::random_device rd;
	rng_ = std::mt19937(rd());

	// グリッドシャッフルリストを構築
	BuildSpawnList();

	// ダメージVignette演出が残っている場合は即時キャンセル
	auto bc = scene->GetBattleController();
	if (bc && bc->GetPlayer()) {
		bc->GetPlayer()->ResetDamageEffect();
	}

	auto sm = SceneManager::GetInstance();
	sm->ClearPostEffects();
	sm->SetCRTShutDownProgress(0.0f);

	// シャットダウンSE登録（読み込み済みならキャッシュIDが返る）
	shutdownSEId_ = SoundManager::GetInstance()->LoadSound("resources/sounds/se/test2.mp3");
}

void GamePhaseGameOver::SpawnWindow() {
	if (spawnCount_ >= static_cast<int>(spawnList_.size())) return;

	const SpawnCell& cell = spawnList_[spawnCount_];

	ErrorWindow win;
	win.spawnTime = timer_;
	win.alpha     = 0.0f;
	win.scaleAnim = 0.0f;

	// 本体（赤い矩形）
	win.body = std::make_unique<Sprite>();
	win.body->Initialize("default.png");
	win.body->SetSize({ cell.w, cell.h });
	win.body->SetTranslate({ cell.x, cell.y });
	win.body->SetColor({ 0.75f, 0.05f, 0.05f, 0.0f });

	// タイトルバー（上部、少し暗い赤）
	win.titleBar = std::make_unique<Sprite>();
	win.titleBar->Initialize("default.png");
	win.titleBar->SetSize({ cell.w, 28.0f });
	win.titleBar->SetTranslate({ cell.x, cell.y });
	win.titleBar->SetColor({ 0.55f, 0.02f, 0.02f, 0.0f });

	windows_.push_back(std::move(win));
}

void GamePhaseGameOver::Update(GameScene* scene, float deltaTime) {
	(void)scene;
	timer_ += deltaTime;

	// ウィンドウを一定間隔でスポーン（全グリッドセル分）
	if (spawnCount_ < static_cast<int>(spawnList_.size()) && timer_ >= nextSpawnTimer_) {
		SpawnWindow();
		spawnCount_++;
		nextSpawnTimer_ = timer_ + kSpawnInterval-(kSpawnInterval*spawnCount_) * 0.05f;
	}

	// 各ウィンドウのポップインアニメーション
	for (auto& win : windows_) {
		float age = timer_ - win.spawnTime;
		if (age < kPopInDuration) {
			win.scaleAnim = age / kPopInDuration;
		} else {
			win.scaleAnim = 1.0f;
		}
		// フェードイン（ポップインに合わせてアルファを上げる）
		win.alpha = std::min(win.scaleAnim, 1.0f);

		// スプライトに反映
		Vector4 bodyCol  = { 0.75f, 0.05f, 0.05f, win.alpha * 0.92f };
		Vector4 barCol   = { 0.55f, 0.02f, 0.02f, win.alpha * 0.95f };

		// スケールアニメーション: overshoot でポップイン感を演出
		// ease-out back 近似: t * (1.1 - 0.1 * cos(t * PI))
		float sc = win.scaleAnim < 1.0f
			? win.scaleAnim + 0.18f * std::sin(win.scaleAnim * 3.14159f)
			: 1.0f;

		win.body->SetColor(bodyCol);
		win.body->SetScale({ sc, sc });
		win.body->Update();

		win.titleBar->SetColor(barCol);
		win.titleBar->SetScale({ sc, sc });
		win.titleBar->Update();
	}

	

	// 演出完了 → CRT電源断（プツン）で ResultScene へ（タイトル遷移と同じ演出）
	// 全ウィンドウがスポーンし、最後の1枚のポップインが終わったらシャットダウン開始
	bool allWindowsShown = spawnCount_ >= static_cast<int>(spawnList_.size())
		&& !windows_.empty() && windows_.back().scaleAnim >= 1.0f;
	if (!sceneChangeRequested_ && allWindowsShown) {
		sceneChangeRequested_ = true;
		// BGM停止とシャットダウンSE再生
		BgmPlayer::GetInstance()->Stop();
		if (!SePlayer::GetInstance()->IsPlaying(shutdownSEId_)) {
			SePlayer::GetInstance()->Play(shutdownSEId_, 1.0f);
		}

		TransitionDesc desc;
		desc.outType     = TransitionType::kCRTShutDown;
		desc.outDuration = kShutDownDuration;
		desc.waitDuration = kShutDownWaitDuration;
		desc.inType      = TransitionType::kFadeBlack;
		desc.inDuration  = 0.6f;
		SceneManager::GetInstance()->RequestSceneChange("ResultScene", desc);
	}
}

void GamePhaseGameOver::Draw(GameScene* scene) {
	(void)scene;
	for (auto& win : windows_) {
		if (win.body)    win.body->Draw();
		if (win.titleBar) win.titleBar->Draw();
	}
}

void GamePhaseGameOver::Exit(GameScene* scene) {
	(void)scene;
	// ビネットをリセット
	auto sm = SceneManager::GetInstance();
	sm->SetFullScreenVignette(false);
	sm->SetFullScreenVignetteScale(0.5f);
	sm->SetFullScreenVignettePower(1.5f);
	windows_.clear();
}

}
