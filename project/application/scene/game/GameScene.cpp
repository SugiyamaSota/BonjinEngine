#include "GameScene.h"

#include"../../others/Collision.h"
#include"../../others/Data.h"
#include<algorithm>

using namespace BonjinEngine;

void GameScene::Initialize(Camera * camera) {
    // 今のシーンと遷移後シーン(初期値は同じ)
    currentSceneType_ = SceneType::kGame;
    nextSceneType_ = SceneType::kGame;

    this->camera_ = camera;

	// マップチップフィールド
	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadmapChipCsv("resources/maps/tutrial.csv");

	// プレイヤー
	model_ = std::make_unique<Model>();
	model_->LoadModel("player");
	player_ = std::make_unique<Player>();
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 1);
	player_->Initialize(model_.get(), camera_, playerPosition);
	player_->SetMapChipField(mapChipField_.get());
	player_->SetLockedOnEnemiesList(&lockedOnEnemies_);

	// カメラのターゲット座標をプレイヤーの初期座標に設定
	cameraTarget_ = playerPosition;

	// 敵
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kEnemy) {
				// 新しいモデルを生成
				enemyModels_.push_back(std::make_unique<Model>());
				enemyModels_.back()->LoadModel("enemy");

				// 新しい敵を生成
				enemies_.push_back(std::make_unique<Enemy>());
				// 敵の位置を縦に並べる
				Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
				// 生成したモデルを敵に渡して初期化
				enemies_.back()->Initialize(enemyModels_.back().get(), camera_, enemyPosition);
			}
		}
	}

	// ブロック
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			blockModel_[i][j] = new Model();
			blockModel_[i][j]->LoadModel("cube");
			blockWorldTransform_[i][j].rotate = { 0,0,0 };
			blockWorldTransform_[i][j].scale = { 1,1,1 };
			blockWorldTransform_[i][j].translate = { 0,0,0 };
		}
	}

	// ゴール
	goalModel_ = new Model();
	goalModel_->LoadModel("goal");
	goalWorldTransform_ = InitializeWorldTransform();
	isGoal_ = false;

	GenerateBlocksAndGoal();

	// 天球
	skydomeModel_ = std::make_unique<Model>();
	skydomeModel_->LoadModel("debugSkydome");
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(skydomeModel_.get(), camera_);

	HUD = new Sprite();
	HUD->Initialize({ 640.0f,360.0f,0.0f }, Color::White, { 0.5f,0.5f,0.0f }, { 1280,720 }, "HUD.png");

	for (int i = 1; i <= 4; ++i) {
		std::string filename = "tutrial" + std::to_string(i) + ".png";
		std::unique_ptr<Sprite> sprite = std::make_unique<Sprite>();
		sprite->Initialize({ 640.0f,360.0f,0.0f }, Color::White, { 0.5f,0.5f,0.0f }, { 1280,720 }, filename);
		tutrialSprites_.push_back(std::move(sprite));
	}

	showTutrial = false;
	currentTutrialPage_ = 0;

	gameClearSprite_ = new Sprite();
	gameClearSprite_->Initialize({ 640.0f,360.0f,0.0f }, Color::White, { 0.5f,0.5f,0.0f }, { 1280,720 }, "GameClear.png");

	blackScreenSprite_ = new Sprite();
	blackScreenSprite_->Initialize({ 640.0f,360.0f,0.0f }, Color::Black, { 0.5f,0.5f,0.0f }, { 1280,720 }, "uvChecker.png");
	blackScreenSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

	sceneChangeStandby_ = false;

	// フェーズの初期化
	phase_ = GamePhase::kStart;
	phaseTimer_ = 0.0f;

	// カメラ
	camera_->SetTarget(goalWorldTransform_.translate);
	camera_->Update(Camera::CameraType::kNormal);

	fadeInAlpha = 1.0f;

	// 天球の更新
	skydome_->Update();

	player_->UpdateWorldTransform();

	// 敵
		// リスト内のすべての敵を更新
	for (const auto& enemy : enemies_) {
		enemy->Update();
	}

	// 破片の更新と削除
	for (auto it = debris_.begin(); it != debris_.end();) {
		if ((*it)->GetIsDead()) {
			it = debris_.erase(it);
		} else {
			(*it)->Update();
			++it;
		}
	}

	// ブロック
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockModel_[i][j]->Update(blockWorldTransform_[i][j], camera_);
			}
		}
	}

	goalModel_->Update(goalWorldTransform_, camera_);
}

GameScene::~GameScene() {
	// ブロック
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			delete blockModel_[i][j];
		}
	}

	delete goalModel_;
	delete HUD;
	delete gameClearSprite_;
	delete blackScreenSprite_;

	for (Enemy* enemyPtr : lockedOnEnemies_) {
		delete enemyPtr; // Enemyオブジェクトを解放
	}

	// 2. リスト自体をクリア
	lockedOnEnemies_.clear();
}

void GameScene::Update(float deltaTime) {
	switch (phase_) {
	case GamePhase::kStart:
		// フェーズ開始からの時間を計測
		phaseTimer_ += 1.0f / 60.0f;

		// フェードインのアルファ値を計算（2秒かけて不透明から透明へ）
		fadeInAlpha = 1.0f - min(phaseTimer_ / 2.0f, 1.0f);
		blackScreenSprite_->Update({ 640.0f,360.0f,0.0f }, Color::Black);
		blackScreenSprite_->SetColor({ 0.0f, 0.0f, 0.0f, fadeInAlpha });

		// フェードインが完了（2秒経過）したら、カメラ演出を開始
		if (phaseTimer_ >= 2.0f) {
			// カメラ演出の補間率を計算
			Vector3 goalPos = goalWorldTransform_.translate;
			Vector3 playerPos = player_->GetPosition();

			// 残りの演出時間を計算（kStartTime - 2.0f）
			float t = (phaseTimer_ - 2.0f) / (kStartTime - 2.0f);
			t = min(t, 1.0f); // 0.0から1.0の範囲にクランプ

			camera_->SetTarget(Lerp(goalPos, playerPos, t));

			// カメラ演出が完了したら次のフェーズへ
			if (t >= 1.0f) {
				phase_ = GamePhase::kPlay;
			}
		}
		break;
	case GamePhase::kPlay:

		if (showTutrial == false) {
			player_->Update();
			HUD->Update({ 640.0f,360.0f,0.0f }, Color::White);
		} else {
			// 左右の入力でチュートリアルページを切り替える
			if (Input::GetInstance()->IsPadTrigger(1)) {
				// 前のページへ、ただし0より小さくならないように
				currentTutrialPage_ = max(0, currentTutrialPage_ - 1);
			}
			if (Input::GetInstance()->IsPadTrigger(0)) {
				// 次のページへ、ただし最後のページを超えないように
				currentTutrialPage_ = min(static_cast<int>(tutrialSprites_.size() - 1), currentTutrialPage_ + 1);
			}

			// 現在選択されているチュートリアルスプライトのみを更新
			tutrialSprites_[currentTutrialPage_]->Update({ 640.0f,360.0f,0.0f }, Color::White);
		}

		// ゴール判定
		CheckGoal();

		// 当たり判定
		CheckAllCollisions();

		// カメラのターゲットを線形補間
		cameraTarget_ = Lerp(cameraTarget_, player_->GetPosition(), kCameraLerpRate);
		camera_->SetTarget(cameraTarget_);
		camera_->Update(Camera::CameraType::kNormal);



		// 敵を死亡状態のものを削除
		enemies_.remove_if([&](const std::unique_ptr<Enemy>& enemy) {
			if (enemy->GetIsDead()) {
				// 敵が死んだら、破片を生成して取得
				std::list<std::unique_ptr<Debris>> newDebris = enemy->ExplodeAndGetDebris();
				// 取得した破片をGameSceneのリストに移動
				debris_.splice(debris_.end(), std::move(newDebris));
				lockedOnEnemies_.remove(enemy.get());
				return true;
			}
			return false;
			});

		// 操作方法の表示操作
		if (Input::GetInstance()->IsPadTrigger(7)) {
			if (showTutrial == false) {
				showTutrial = true;
			} else {
				showTutrial = false;
			}
		}
		break;
	case GamePhase::kGoal:
		// 常にゲームクリアスプライトを更新
		gameClearSprite_->Update({ 640.0f,360.0f,0.0f }, Color::White);

		// タイマーをインクリメント
		phaseTimer_ += 1.0f / 60.0f;

		// ゴール後1秒経過したらフェード開始
		if (phaseTimer_ >= 1.0f) {
			// フェードインのタイマーを更新
			fadeTimer_ += 1.0f / 60.0f;
			// アルファ値を0から1へ徐々に増加させる（例: 2秒かけてフェードイン）
			float alpha = min(fadeTimer_ / 2.0f, 1.0f);
			blackScreenSprite_->Update({ 640.0f,360.0f,0.0f }, Color::White);
			blackScreenSprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha });
			// フェード完了後、シーン変更待機状態へ
			if (alpha >= 1.0f) {
				nextSceneType_ = SceneType::kTitle;
			}
		}
		// カメラをプレイヤーの位置に固定
		camera_->SetPosition(player_->GetPosition());
		break;
	}

	if (showTutrial == false) {
		// 天球の更新
		skydome_->Update();

		player_->UpdateWorldTransform();

		// 敵
			// リスト内のすべての敵を更新
		for (const auto& enemy : enemies_) {
			enemy->Update();
		}

		// 破片の更新と削除
		for (auto it = debris_.begin(); it != debris_.end();) {
			if ((*it)->GetIsDead()) {
				it = debris_.erase(it);
			} else {
				(*it)->Update();
				++it;
			}
		}

		// ブロック
		for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
			for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
				if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
					blockModel_[i][j]->Update(blockWorldTransform_[i][j], camera_);
				}
			}
		}

		goalModel_->Update(goalWorldTransform_, camera_);
	}
}

void GameScene::Draw() {
	// 天球の描画
	skydome_->Draw();

	// 自キャラの描画
	player_->Draw();

	// ブロックの描画
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockModel_[i][j]->Draw();
			}
		}
	}

	// 敵
	// リスト内のすべての敵を描画
	for (const auto& enemy : enemies_) {
		enemy->Draw();
	}

	// 破片の描画
	for (const auto& debris : debris_) {
		debris->Draw();
	}

	goalModel_->Draw();

	if (showTutrial == false) {
		HUD->Draw();
	} else {
		tutrialSprites_[currentTutrialPage_]->Draw();
	}

	if (isGoal_ == true) {
		gameClearSprite_->Draw();
	}
	// 黒いスプライトを一番手前に描画
	blackScreenSprite_->Draw();
}

SceneType GameScene::GetNextScene() const {
    return nextSceneType_;
}

void GameScene::GenerateBlocksAndGoal() {
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVirtical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;

	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockWorldTransform_[i][j].rotate = { 0,0,0 };
				blockWorldTransform_[i][j].scale = { 1,1,1 };
				blockWorldTransform_[i][j].translate = mapChipField_->GetMapChipPositionByIndex(j, i);
			} else
				if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kGoal) {
					goalWorldTransform_.rotate = { 0,0,0 };
					goalWorldTransform_.scale = { 1,1,1 };
					goalWorldTransform_.translate = mapChipField_->GetMapChipPositionByIndex(j, i);
					goalPosition = goalWorldTransform_.translate;
				}
		}
	}
}

void GameScene::CheckAllCollisions() {
	// AABB変数の宣言
	AABB playerAABB, enemyAABB, anchorAABB;

	// プレイヤーのAABBを取得
	playerAABB = player_->GetAABB();

	// リスト内の各敵と当たり判定
	for (const auto& enemy : enemies_) {
		// 敵のAABBを取得
		enemyAABB = enemy->GetAABB();

		// プレイヤーと敵の当たり判定
		if (IsCollision(playerAABB, enemyAABB)) {
			player_->OnCollision(enemy.get());
		}

		// アンカーと敵の当たり判定
		if (player_->HasAnchor()) {
			anchorAABB = player_->GetAnchor().GetAABB();
			if (IsCollision(anchorAABB, enemyAABB)) {
				player_->GetAnchor().OnCollision();

				// ロックオン処理
				// この敵がまだロックオンされていない場合のみ処理を行う
				if (!enemy->GetIsLockedOn()) {
					// ロックオンされた敵の数が上限に達しているかチェック
					if (lockedOnEnemies_.size() >= kMaxLockedOnEnemies) {
						// 一番古いロックオンを解除
						Enemy* oldestEnemy = lockedOnEnemies_.front();
						oldestEnemy->SetIsLockedOn(false);
						lockedOnEnemies_.pop_front();
					}

					// 新しい敵をロックオンリストに追加
					enemy->SetIsLockedOn(true);
					lockedOnEnemies_.push_back(enemy.get());
				}
			}
		}
	}
}

void GameScene::CheckGoal() {
	IndexSet playerIndexSet = mapChipField_->GetMapChipIndexSetByPosition(player_->GetPosition());
	MapChipType playerMapChip = mapChipField_->GetMapChipTypeByIndex(playerIndexSet.xIndex, playerIndexSet.yIndex);

	// プレイヤーがゴール地点にいるか判定
	if (playerMapChip == MapChipType::kGoal) {
		// ゴールした瞬間にタイマーをリセット
		if (!isGoal_) { // 既にゴール済みでないかチェック
			phaseTimer_ = 0.0f;
		}
		isGoal_ = true;
		phase_ = GamePhase::kGoal;
	}
}