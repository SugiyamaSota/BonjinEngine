#include "GameScene.h"

#include"../system/utility/random/RandomEngine.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = SceneType::kGame;
	nextSceneType_ = SceneType::kGame;
	ChangePhase(GamePhase::kStart);
	

	this->camera_ = camera;

	// マップチップフィールド
	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadmapChipCsv("resources/maps/tutorial.csv");

	

	fadeIOAlpha_ = 1.0f;

	PlayerInit();

	EnemyInit();

	BlocksAndGoalInit();

	HUDInit();

	// カメラ
	camera_->SetTarget(goalWorldTransform_.translate);
	camera_->Update(Camera::CameraType::kDebug);

	lightMngr_ = LightManager::GetInstance();

	lightMngr_->SpotLightData().direction = { 0,-1,0 };
	lightMngr_->SpotLightData().intensity = 1000.f;
	lightMngr_->SpotLightData().color = { 1.f,0.f,0.f,1.f };

}

void GameScene::Unload() {
}

void GameScene::Update(float deltaTime) {
	UpdatePhaseTimer(deltaTime);

	switch (phase_) {
	case GamePhase::kStart:
		

		// フェードインのアルファ値を計算（2秒かけて不透明から透明へ）
		fadeIOAlpha_ = 1.0f - min(phaseTimer_ / 2.0f, 1.0f);

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

		// 敵生存確認
		if (enemies_.empty()) {
			canGoal_ = true;
			lightMngr_->SpotLightData().color = { 0.f,1.f,0.f,1.f };
		}

		if (player_->firstStep_ == true && showFirstTutrial_ == false) {
			showTutrial = true;
			showFirstTutrial_ = true;
		}

		if (showTutrial == false) {
			player_->Update();
			HUD_Tab_->Update();
			HUD_Underbar_->Update();
			HUD_Default_->Update();
			HUD_Anchor_->Update();
			HUD_Destroy_->Update();
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
			tutrialSprites_[currentTutrialPage_]->Update();
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
		if (Input::GetInstance()->IsPadTrigger(7)||Input::GetInstance()->IsTrigger(DIK_TAB)) {
			if (showTutrial == false) {
				showTutrial = true;
			} else {
				showTutrial = false;
			}
		}
		break;
	case GamePhase::kGoal:
		// 常にゲームクリアスプライトを更新
		gameClearSprite_->Update();

		// タイマーをインクリメント
		phaseTimer_ += 1.0f / 60.0f;

		// ゴール後1秒経過したらフェード開始
		if (phaseTimer_ >= 1.0f) {
			// フェードインのタイマーを更新
			fadeTimer_ += 1.0f / 60.0f;
			// アルファ値を0から1へ徐々に増加させる（例: 2秒かけてフェードイン）
			fadeIOAlpha_ = min(fadeTimer_ / 2.0f, 1.0f);
			
			// フェード完了後、シーン変更待機状態へ
			if (fadeIOAlpha_ >= 1.0f) {
				nextSceneType_ = SceneType::kTitle;
			}
		}
		// カメラをプレイヤーの位置に固定
		camera_->SetPosition(player_->GetPosition());
		break;
	}

	if (showTutrial == false) {
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

		if (canGoal_) {
			goalModel_->SetColor(Vector4(1.f, 1.f, 1.f, 1.f));
		} else {
			goalModel_->SetColor(Vector4(1.f, 1.f, 1.f, 0.3f));
		}

		goalModel_->Update(goalWorldTransform_, camera_);
	}
}

void GameScene::Draw() {

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
		HUD_Tab_->Draw();
		DrawHUD();
	} else if (showTutrial == true) {
		tutrialSprites_[currentTutrialPage_]->Draw();
	}

	if (isGoal_ == true) {
		gameClearSprite_->Draw();
	}

}


void GameScene::DrawSceneImGui() {

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

					LightManager* lightMngr_ = LightManager::GetInstance();
					Vector3 currentLightPosition = mapChipField_->GetMapChipPositionByIndex(j, i - 1);
					lightMngr_->SpotLightData().position = currentLightPosition;
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
	if (playerMapChip == MapChipType::kGoal && canGoal_ == true) {
		// ゴールした瞬間にタイマーをリセット
		if (!isGoal_) { // 既にゴール済みでないかチェック
			phaseTimer_ = 0.0f;
		}
		isGoal_ = true;
		phase_ = GamePhase::kGoal;
	}
}

void GameScene::DrawHUD() {


	// 常に描画
	HUD_Underbar_->Draw();
	HUD_Default_->Draw();

	// アンカーが存在するなら
	if (player_->HasAnchor()) {
		if (player_->GetAnchor().GetStandBy()) {
			HUD_Anchor_->Draw();
		}
	}

	if (lockedOnEnemies_.size() > 0) {
		HUD_Destroy_->Draw();
	}

}

void GameScene::PlayerInit() {
	// プレイヤー
	playerModel_ = std::make_unique<Model>();
	playerModel_->LoadModel("player");
	player_ = std::make_unique<Player>();
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 1);
	player_->Initialize(playerModel_.get(), camera_, playerPosition);
	player_->SetMapChipField(mapChipField_.get());
	player_->SetLockedOnEnemiesList(&lockedOnEnemies_);

	player_->Update();

	// カメラのターゲット座標をプレイヤーの初期座標に設定
	/*cameraTarget_ = playerPosition;
	camera_->Update(Camera::CameraType::kNormal);*/
}

void GameScene::EnemyInit() {
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

	for (const auto& enemy : enemies_) {
		enemy->Update();
	}

}

void GameScene::BlocksAndGoalInit() {
	// ブロック
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			blockWorldTransform_[i][j].rotate = { 0,0,0 };
			blockWorldTransform_[i][j].scale = { 1,1,1 };
			blockWorldTransform_[i][j].translate = { 0,0,0 };
			blockModel_[i][j] = std::make_unique<Model>();
			blockModel_[i][j]->LoadModel("cube");
		}
	}

	// ゴール
	goalWorldTransform_ = InitializeWorldTransform();
	goalModel_ = std::make_unique<Model>();
	goalModel_->LoadModel("goal");
	goalModel_->SetColor(Vector4(1.f, 1.f, 1.f, 0.3f));
	goalModel_->SetBlendMode(BlendMode::kAdd);
	goalModel_->Update(goalWorldTransform_, camera_);

	isGoal_ = false;

	GenerateBlocksAndGoal();

	// ブロック
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockModel_[i][j]->Update(blockWorldTransform_[i][j], camera_);
			}
		}
	}
}

void GameScene::HUDInit() {
	HUD_Tab_ = std::make_unique<Sprite>();
	HUD_Tab_->Initialize("HUD.png");
	HUD_Tab_->Scale() = { 1.f,1.f };
	HUD_Tab_->Rotate() = { 0.f,0.f };
	HUD_Tab_->Translate() = { 640.f, 360.f};
	HUD_Tab_->Anchor() = { 0.5f,0.5f };
	HUD_Tab_->Size() = { 1280.f, 720.f };

	HUD_Underbar_ = std::make_unique<Sprite>();
	HUD_Underbar_->Initialize("HUD_Underbar.png");
	HUD_Underbar_->Scale() = { 1.f,1.f };
	HUD_Underbar_->Rotate() = { 0.f,0.f };
	HUD_Underbar_->Translate() = { 640.f, 360.f };
	HUD_Underbar_->Anchor() = { 0.5f,0.5f };
	HUD_Underbar_->Size() = { 1280.f, 720.f };

	HUD_Default_ = std::make_unique<Sprite>();
	HUD_Default_->Initialize("HUD_Default.png");
	HUD_Default_->Scale() = { 1.f,1.f };
	HUD_Default_->Rotate() = { 0.f,0.f };
	HUD_Default_->Translate() = { 640.f, 360.f };
	HUD_Default_->Anchor() = { 0.5f,0.5f };
	HUD_Default_->Size() = { 1280.f, 720.f };

	HUD_Anchor_ = std::make_unique<Sprite>();
	HUD_Anchor_->Initialize("HUD_Anchor.png");
	HUD_Anchor_->Scale() = { 1.f,1.f };
	HUD_Anchor_->Rotate() = { 0.f,0.f };
	HUD_Anchor_->Translate() = { 640.f, 360.f };
	HUD_Anchor_->Anchor() = { 0.5f,0.5f };
	HUD_Anchor_->Size() = { 1280.f, 720.f };

	HUD_Destroy_ = std::make_unique<Sprite>();
	HUD_Destroy_->Initialize("HUD_Destroy.png");
	HUD_Destroy_->Scale() = { 1.f,1.f };
	HUD_Destroy_->Rotate() = { 0.f,0.f };
	HUD_Destroy_->Translate() = { 640.f, 360.f };
	HUD_Destroy_->Anchor() = { 0.5f,0.5f };
	HUD_Destroy_->Size() = { 1280.f, 720.f };


	for (int i = 1; i <= 4; ++i) {
		std::string filename = "tutorial" + std::to_string(i) + ".png";
		std::unique_ptr<Sprite> sprite = std::make_unique<Sprite>();
		sprite->Initialize(filename);
		sprite->Scale() = { 1.f,1.f };
		sprite->Rotate() = { 0.f,0.f };
		sprite->Translate() = { 640.f, 360.f };
		sprite->Anchor() = { 0.5f,0.5f };
		sprite->Size() = { 1280.f, 720.f };
		tutrialSprites_.push_back(std::move(sprite));
	}

	gameClearSprite_ = std::make_unique<Sprite>();
	gameClearSprite_->Initialize("GameClear.png");
	gameClearSprite_->Scale() = { 1.f,1.f };
	gameClearSprite_->Rotate() = { 0.f,0.f };
	gameClearSprite_->Translate() = { 640.f, 360.f };
	gameClearSprite_->Anchor() = { 0.5f,0.5f };
	gameClearSprite_->Size() = { 1280.f, 720.f };

	showTutrial = false;
	currentTutrialPage_ = 0;
}