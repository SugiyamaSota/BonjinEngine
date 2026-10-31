#include "GameOver.h"
#include "../GameScene.h"

#include "SceneManager.h"

#include <algorithm>
#include <cmath>

namespace Bonjin::GamePhase {

void GameOver::BuildSpawnList() {
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

void GameOver::Enter(GameScene* scene) {
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

void GameOver::SpawnWindow() {
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

void GameOver::Update(GameScene* scene, float deltaTime) {
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

void GameOver::Draw(GameScene* scene) {
	(void)scene;
	for (auto& win : windows_) {
		if (win.body)    win.body->Draw();
		if (win.titleBar) win.titleBar->Draw();
	}
}

void GameOver::Exit(GameScene* scene) {
	(void)scene;
	// ビネットをリセット
	auto sm = SceneManager::GetInstance();
	sm->SetFullScreenVignette(false);
	sm->SetFullScreenVignetteScale(0.5f);
	sm->SetFullScreenVignettePower(1.5f);
	windows_.clear();
}

}
