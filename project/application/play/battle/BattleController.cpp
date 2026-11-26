#include "BattleController.h"
#include "gameObject/camera/CameraController.h"

#include "logic/Collision.h"
#include "logic/CollisionManager.h"
#include "gameObject/anchor/anchor.h"
#include "gameObject/enemy/EnemyFactory.h"
#include "GpuParticleManager.h"
#include "SceneManager.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "vector.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <chrono>
#include <sstream>

using namespace Bonjin;

void BattleController::Initialize(Camera* camera, const char* mapFilePath) {
	auto tStart = std::chrono::high_resolution_clock::now();
	CollisionManager::GetInstance()->Clear();

	camera_ = camera;
	enemies_.clear();
	enemyModels_.clear();
	lockedOnEnemies_.clear();

	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadStage(mapFilePath);

	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();

	blockModels_.resize(vertical);
	blockWorldTransforms_.resize(vertical);
	for (uint32_t i = 0; i < vertical; ++i) {
		blockModels_[i].resize(horizontal);
		blockWorldTransforms_[i].resize(horizontal);
	}

	isGoalReached_ = false;

	GenerateBlocksAndGoal();

	playerModel_ = std::make_unique<Object3D>();
	playerModel_->SetModel("player");
	playerModel_->SetEnableEnableEnvironmentMap(false);
	playerModel_->SetCullMode(CullMode::kFront);

	player_ = std::make_unique<Player>();
	const Vector2 pSpawn = mapChipField_->GetPlayerSpawn();
	const Vector3 playerPosition = mapChipField_->GetMapChipPosition(pSpawn.x, pSpawn.y);
	player_->Initialize(playerModel_.get(), camera_, playerPosition);
	player_->SetMapChipField(mapChipField_.get());
	// プレイヤーは2マス分の高さがあり、マスの中心に置くと足が床に埋まるので押し出す
	player_->ResolveMapEmbedding();
	player_->SetLockedOnEnemiesList(&lockedOnEnemies_);
	player_->Update();

	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(camera_, player_->GetPosition());

	// 敵の生成
	const auto& enemySpawns = mapChipField_->GetEnemySpawns();
	for (const auto& spawn : enemySpawns) {
		enemyModels_.push_back(std::make_unique<Object3D>());
		enemyModels_.back()->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
		enemyModels_.back()->SetEnableEnableEnvironmentMap(false);

		const Vector3 enemyPosition = mapChipField_->GetMapChipPosition(spawn.x, spawn.y);

		// 種類ごとの違い (色・出現する高さ・動き) はそれぞれの敵クラスが持っている
		std::unique_ptr<BaseEnemy> enemy = EnemyFactory::Create(spawn.type);
		enemy->Initialize(enemyModels_.back().get(), camera_, enemyPosition);
		enemy->SetMapChipField(mapChipField_.get());
		enemy->SetPlayer(player_.get());
		enemy->Update();
		enemies_.push_back(std::move(enemy));
	}

	skyBox_ = std::make_unique<SkyBox>();
	skyBox_->CreateModel(
		ModelBuilder::ModelType::kSkyBox, "resources/textures/SkyBox_Dark.dds");

	particleManager_ = ParticleManager::GetInstance();
	particleManager_->Initialize();
	particleManager_->CreateParticleGroup(
		"anchorHitRay", ModelBuilder::ModelType::kPlane, "resources/textures/gradationLine.png");
	particleManager_->CreateParticleGroup(
		"anchorHitFlash", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager_->CreateParticleGroup(
		"enemyDefeatSpark", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager_->CreateParticleGroup(
		"enemyDefeatRing", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");
	particleManager_->CreateParticleGroup(
		"enemyChargeRing", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");
	particleManager_->CreateParticleGroup(
		"landingDust", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager_->Clear("anchorHitRay");
	particleManager_->Clear("anchorHitFlash");
	particleManager_->Clear("enemyDefeatSpark");
	particleManager_->Clear("enemyDefeatRing");
	particleManager_->Clear("landingDust");
	// ゴールのポータル (光の柱 + 昇るリング)。グループとテクスチャは Core.cpp で起動時に作成済み
	Goal::ClearPortalParticles();
	if (goal_) {
		goal_->EmitPortalPillar();
	}


	hud_ = std::make_unique<HUD>();
	hud_->Initialize();

	anchorStockIndicator_ = std::make_unique<AnchorStockIndicator>();
	anchorStockIndicator_->Initialize();

	goSignalBanner_ = std::make_unique<GoSignalBanner>();
	goSignalBanner_->Initialize();

	GpuParticleManager::GetInstance()->Clear();

	if (player_) {
		player_->UpdateModel();
	}

	auto tEnd = std::chrono::high_resolution_clock::now();
	float initMs = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
	std::stringstream ss;
	ss << "[BattleController::Initialize] Total took: " << initMs << " ms\n";
	OutputDebugStringA(ss.str().c_str());
}

void BattleController::Unload() {
	// 出しっぱなしのポータルの柱を次のシーンに持ち越さない
	if (particleManager_) {
		Goal::ClearPortalParticles();
	}
	// クリア演出で寄せたカメラ距離を次のシーンに持ち越さない
	if (cameraController_) {
		cameraController_->ResetCameraRadius();
	}
	if (goSignalBanner_) {
		goSignalBanner_.reset();
	}
	GpuParticleManager::GetInstance()->Clear();
	hud_.reset();
	anchorStockIndicator_.reset();
	lockedOnEnemies_.clear();
	enemies_.clear();
	enemyModels_.clear();
	player_.reset();
	playerModel_.reset();
	goal_.reset();
	blockModels_.clear();
	blockWorldTransforms_.clear();
	mapChipField_.reset();
	skyBox_.reset();
	particleManager_ = nullptr;
	camera_ = nullptr;
}

void BattleController::Update(float deltaTime) {
	// ヒットストップ中の処理（時間停止中ならカメラをプレイヤーに固定し、全ビジュアルをフリーズ状態で同期）
	if (hitStopTimer_ > 0.0f) {
		hitStopTimer_ -= deltaTime;
		if (hitStopTimer_ < 0.0f) {
			hitStopTimer_ = 0.0f;
		}

		if (cameraController_) {
			cameraController_->SetTargetInstant(player_->GetWorldPosition());
		}
		UpdateVisuals(0.0f);
		return;
	}

	(void)deltaTime;
	skyBox_->Update(InitializeWorldTransform(), camera_);

	// プレイヤー更新（テレポート実行）
	player_->Update();

	// テレポート等によりヒットストップがリクエストされた場合、その場で即座にカメラを合わせ、テレポート後視点で完全同期フリーズ
	float hitStopDuration = 0.0f;
	if (player_ && player_->ConsumeHitStopRequest(hitStopDuration)) {
		hitStopTimer_ = hitStopDuration;
		if (cameraController_) {
			cameraController_->SetTargetInstant(player_->GetWorldPosition());
		}
		UpdateVisuals(0.0f);
		return;
	}

	// アンカーの即着地処理
	if (player_->HasAnchor()) {
		Anchor& anchor = player_->GetAnchor();
		if (!anchor.GetStandBy() && !anchor.IsInstantResolved()) {
			ResolveAnchorInstantLanding(anchor);
		}
	}

	UpdateGoal(deltaTime);

	if (player_->ConsumeLandingEffectRequest()) {
		Vector3 dustPosition = player_->GetPosition();
		dustPosition.y -= player_->GetAABB().max.y - player_->GetPosition().y; // 足元 (高さの半分だけ下)
		dustPosition.z = -0.1f;
		EmitLandingDustEffect(dustPosition);
	}

	// 共通衝突判定の実行
	CollisionManager::GetInstance()->CheckAllCollisions();

	// プレイヤーのゴール到達フラグチェック
	if (player_ && player_->GetIsGoalReached()) {
		isGoalReached_ = true;
	}

	for (const auto& enemy : enemies_) {
		if (enemy->ConsumeDefeatEffectRequest()) {
			EmitEnemyDefeatEffect(enemy->GetWorldPosition());
		}

		if (enemy->GetIsDead()) {
			enemy->UpdateRespawn(deltaTime, enemyRespawnTime_, isEnemyRespawnEnabled_);
		} else {
			enemy->Update();
		}
	}

	//const IndexSet centerIndex = mapChipField_->GetMapChipIndexSetByCenter();
	cameraController_->Update(player_->GetWorldPosition(), deltaTime);

	// ImGuiのパーティクル確認 (自動発生)
	UpdateParticlePreview(deltaTime);

	// ImGuiのテスト再生でクリア演出が動いている間もカメラ演出を付ける
	if (player_->GetClearPhase() != Player::ClearPhase::kNone) {
		cameraController_->SetCameraRadius(CalculateClearCameraRadius());
		UpdateClearPulseEffects(); // 寄るタイミングでパーティクル
		isClearCameraActive_ = true;
	} else if (isClearCameraActive_) {
		cameraController_->ResetCameraRadius();
		isClearCameraActive_ = false;
	}

	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();
	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
			if (blockModels_[i][j]) {
				blockModels_[i][j]->Update(blockWorldTransforms_[i][j], camera_);
			}
		}
	}

	particleManager_->Update(camera_);
	GpuParticleManager::GetInstance()->Update(deltaTime, camera_);

	if (hud_) {
		hud_->Update(player_.get(), deltaTime);
	}

	if (anchorStockIndicator_) {
		anchorStockIndicator_->Update(player_.get(), camera_, deltaTime);
	}

	if (goSignalBanner_) {
		goSignalBanner_->Update(deltaTime);
	}
}

void BattleController::Draw() {
	skyBox_->Draw();

	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();
	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
			if (blockModels_[i][j]) {
				blockModels_[i][j]->Draw();
			}
		}
	}

	if (goal_) {
		goal_->Draw();
	}

	player_->Draw();
	for (const auto& enemy : enemies_) {
		if (!enemy->GetIsDead()) {
			enemy->Draw();
		}
	}
	particleManager_->Draw();

	if (player_) {
		player_->DrawAnchorLine();
	}

	// GPUパーティクル（雷霆エフェクト・スパーク）の描画
	GpuParticleManager::GetInstance()->Draw(camera_);

	// 2D HUD / UIの描画
	if (isHudVisible_ && hud_) {
		hud_->Draw();
	}

	// プレイヤー頭上のアンカーストック表示
	if (isHudVisible_ && anchorStockIndicator_) {
		anchorStockIndicator_->Draw();
	}

	// 「GO!」演出バナー描画
	if (goSignalBanner_) {
		goSignalBanner_->Draw();
	}
}

void BattleController::DrawImGui() {
#ifdef USE_IMGUI
	LightManager::GetInstance()->DrawImGui();
	cameraController_->DrawImGui();
	ImGui::Checkbox("Enemy Respawn", &isEnemyRespawnEnabled_);
	ImGui::SliderFloat("Enemy Respawn Time", &enemyRespawnTime_, 0.5f, 10.0f, "%.1f sec");

	if (hud_) {
		hud_->DrawImGui();
	}
	if (anchorStockIndicator_) {
		anchorStockIndicator_->DrawImGui();
	}
	if (player_) {
		player_->DrawImGui();
	}

	GpuParticleManager::GetInstance()->DrawImGui();
	DrawParticlePreviewImGui();

	if (ImGui::TreeNode("Enemies")) {
		int index = 0;
		for (const auto& enemy : enemies_) {
			enemy->DrawImGui(index++);
		}
		ImGui::TreePop();
	}

	if (goal_) {
		goal_->DrawImGui();
	}

	if (ImGui::TreeNode("Clear Camera")) {
		ImGui::Text("Default Radius: %.1f", Camera::kDefaultRadius);
		ImGui::DragFloat("Close Radius (FlairStart)", &clearCameraCloseRadius_, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("Pulse Radius (Flairing)", &clearCameraPulseRadius_, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("Pulse In Time", &clearCameraPulseInTime_, 0.005f, 0.0f, 1.0f);
		ImGui::DragFloat("Pulse Out Time", &clearCameraPulseOutTime_, 0.005f, 0.0f, 2.0f);
		ImGui::DragFloat("Pulse Offset", &clearCameraPulseOffset_, 0.01f, 0.0f, 2.0f);
		ImGui::DragFloat("Pulses Per Rotation", &clearCameraPulsesPerRotation_, 0.05f, 0.0f, 8.0f);
		ImGui::SameLine();
		ImGui::TextDisabled("(0で寄らない)");
		if (player_) {
			const float rotation = player_->GetAnimationPlayLength(PlayerAnimator::State::kFlairing);
			if (clearCameraPulsesPerRotation_ > 0.0f && rotation > 0.0f) {
				ImGui::Text("Pulse Interval: %.2fs (1回転 %.2fs)", rotation / clearCameraPulsesPerRotation_, rotation);
			}
		}
		ImGui::SeparatorText("Burst (寄るたびに出すパーティクル)");
		ImGui::SliderInt("Burst Count", &clearBurstCount_, 0, 12);
		ImGui::SliderFloat("Burst Max Tilt (deg)", &clearBurstMaxTiltDeg_, 0.0f, 89.0f, "%.0f");
		if (ImGui::Button("Emit Burst Now")) {
			EmitClearBurst();
		}
		ImGui::TextDisabled("Player > Test Clear Performance で確認できます");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Goal Debug")) {
		ImGui::Text("Goal Reached: %s", isGoalReached_ ? "TRUE" : "FALSE");
		if (goal_) {
			Vector3 goalPos = goal_->GetPosition();
			ImGui::Text("Goal Pos: (%.2f, %.2f, %.2f)", goalPos.x, goalPos.y, goalPos.z);
			AABB pAABB = player_->GetAABB();
			ImGui::Text("Player AABB min: (%.2f, %.2f, %.2f)", pAABB.min.x, pAABB.min.y, pAABB.min.z);
			ImGui::Text("Player AABB max: (%.2f, %.2f, %.2f)", pAABB.max.x, pAABB.max.y, pAABB.max.z);

			AABB gAABB = goal_->GetAABB();
			ImGui::Text("Goal AABB min: (%.2f, %.2f, %.2f)", gAABB.min.x, gAABB.min.y, gAABB.min.z);
			ImGui::Text("Goal AABB max: (%.2f, %.2f, %.2f)", gAABB.max.x, gAABB.max.y, gAABB.max.z);

			bool collision = IsCollision(pAABB, gAABB);
			ImGui::Text("Is Collision: %s", collision ? "TRUE" : "FALSE");
		} else {
			ImGui::Text("Goal is NULL (No Goal in Map)");
		}
		ImGui::TreePop();
	}
#endif
}

void BattleController::GenerateBlocksAndGoal() {
	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();

	Object3D* blockTemplate = nullptr;

	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
			const MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kBlock) {
				blockModels_[i][j] = std::make_unique<Object3D>();
				if (!blockTemplate) {
					blockModels_[i][j]->CreateModel(
						ModelBuilder::ModelType::kCube, "resources/textures/block_metal.png");
					blockTemplate = blockModels_[i][j].get();
				} else {
					blockModels_[i][j]->CopyModelResourceFrom(*blockTemplate);
				}
				blockModels_[i][j]->SetEnableEnableEnvironmentMap(false);
				blockWorldTransforms_[i][j] = InitializeWorldTransform();
				blockWorldTransforms_[i][j].translate =
					mapChipField_->GetMapChipPositionByIndex(j, i);
			} else if (type == MapChipType::kGoal) {
				goal_ = std::make_unique<Goal>();
				goal_->Initialize(camera_, mapChipField_->GetMapChipPositionByIndex(j, i));
			}
		}
	}
}

Vector3 BattleController::GetPreviewPosition() const {
	Vector3 base = previewPosition_;
	if (previewFollowPlayer_ && player_) {
		base = player_->GetPosition();
		// プレイヤーの向いている側にずらす
		const bool facingLeft = std::cos(player_->GetWorldTransform().rotate.y) < 0.0f;
		base.x += facingLeft ? -previewOffset_.x : previewOffset_.x;
		base.y += previewOffset_.y;
		base.z += previewOffset_.z;
		return base;
	}
	return { base.x + previewOffset_.x, base.y + previewOffset_.y, base.z + previewOffset_.z };
}

void BattleController::EmitPreviewEffect(PreviewEffect effect) {
	if (!particleManager_) {
		return;
	}
	const Vector3 position = GetPreviewPosition();
	switch (effect) {
	case PreviewEffect::kAnchorHit:
		EmitAnchorHitEffect(position);
		break;
	case PreviewEffect::kEnemyDefeat:
		EmitEnemyDefeatEffect(position);
		break;
	case PreviewEffect::kLandingDust:
		EmitLandingDustEffect(position);
		break;
	case PreviewEffect::kEnemyChargeRing: {
		// Enemy::ChaseBehavior と同じ設定 (敵の代わりに固定位置で縮む)
		constexpr float kChargeTime = 0.75f;
		constexpr float kMaxScale = 3.0f;
		ParticleConfig config;
		config.position = position;
		config.scale = { kMaxScale, kMaxScale, 1.0f };
		config.color = { 1.0f, 0.0f, 0.0f, 0.5f };
		config.lifeTime = kChargeTime;
		config.updateFunc = [position](ParticleData& data, float) {
			float ratio = (data.lifeTime - data.currentTime) / data.lifeTime;
			ratio = (std::clamp)(ratio, 0.0f, 1.0f);
			data.transform.scale = { ratio * kMaxScale, ratio * kMaxScale, 1.0f };
			data.transform.translate = { position.x, position.y, position.z - 0.05f };
		};
		particleManager_->Emit("enemyChargeRing", config);
		break;
	}
	default:
		break;
	}
}

void BattleController::UpdateParticlePreview(float deltaTime) {
	if (!previewAutoEmit_) {
		previewTimer_ = 0.0f;
		return;
	}
	previewTimer_ -= deltaTime;
	if (previewTimer_ <= 0.0f) {
		previewTimer_ = (std::max)(previewInterval_, 0.05f);
		EmitPreviewEffect(static_cast<PreviewEffect>(previewEffect_));
	}
}

void BattleController::DrawParticlePreviewImGui() {
#ifdef USE_IMGUI
	if (!ImGui::TreeNode("Particle Preview")) {
		return;
	}
	if (!particleManager_) {
		ImGui::TextDisabled("ParticleManager がありません");
		ImGui::TreePop();
		return;
	}

	// 登録グループと使用数
	const uint32_t maxCount = particleManager_->GetMaxCountPerGroup();
	if (ImGui::BeginTable("ParticleGroups", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("Group");
		ImGui::TableSetupColumn("Active");
		ImGui::TableSetupColumn("");
		ImGui::TableHeadersRow();
		for (const std::string& name : particleManager_->GetGroupNames()) {
			const uint32_t active = particleManager_->GetActiveCount(name);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(name.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::ProgressBar(static_cast<float>(active) / static_cast<float>(maxCount), ImVec2(-1.0f, 0.0f),
				(std::to_string(active) + " / " + std::to_string(maxCount)).c_str());
			ImGui::TableSetColumnIndex(2);
			ImGui::PushID(name.c_str());
			if (ImGui::SmallButton("Clear")) {
				particleManager_->Clear(name);
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	ImGui::SeparatorText("Emit");
	// 発生位置
	ImGui::Checkbox("Follow Player", &previewFollowPlayer_);
	if (!previewFollowPlayer_) {
		ImGui::DragFloat3("Base Position", &previewPosition_.x, 0.1f);
		if (player_ && ImGui::SmallButton("Set To Player")) {
			previewPosition_ = player_->GetPosition();
		}
	}
	ImGui::DragFloat3("Offset", &previewOffset_.x, 0.1f);
	const Vector3 position = GetPreviewPosition();
	ImGui::Text("Emit Position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);

	// 演出ごとの発生ボタン
	static const char* kEffectNames[] = {
		"Anchor Hit (anchorHitRay + anchorHitFlash)",
		"Enemy Defeat (enemyDefeatSpark + enemyDefeatRing)",
		"Landing Dust (landingDust)",
		"Enemy Charge Ring (enemyChargeRing)",
	};
	for (int i = 0; i < static_cast<int>(PreviewEffect::kCount); ++i) {
		ImGui::PushID(i);
		if (ImGui::Button("Emit")) {
			EmitPreviewEffect(static_cast<PreviewEffect>(i));
		}
		ImGui::SameLine();
		ImGui::RadioButton(kEffectNames[i], &previewEffect_, i);
		ImGui::PopID();
	}

	// 自動で出し続ける
	ImGui::Checkbox("Auto Emit (selected)", &previewAutoEmit_);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragFloat("Interval", &previewInterval_, 0.01f, 0.05f, 5.0f, "%.2f s");

	if (ImGui::Button("Clear All")) {
		for (const std::string& name : particleManager_->GetGroupNames()) {
			particleManager_->Clear(name);
		}
	}
	ImGui::TextDisabled("※ヒットストップ中やクリア演出中は自動発生しません");

	ImGui::TreePop();
#endif
}

void BattleController::EmitAnchorHitEffect(const Vector3& position) {
	constexpr uint32_t kRayCount = 8;
	std::uniform_real_distribution<float> rotateDistribution(
		-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> lengthDistribution(1.2f, 2.2f);

	for (uint32_t index = 0; index < kRayCount; ++index) {
		ParticleConfig ray{};
		ray.position = position;
		ray.rotate = {0.0f, 0.0f, rotateDistribution(randomEngine_)};
		ray.velocity = {0.0f, 0.0f, 0.0f};
		ray.scale = {0.12f, lengthDistribution(randomEngine_), 1.0f};
		ray.color = {0.35f, 0.75f, 1.0f, 1.0f};
		ray.lifeTime = 0.28f;
		ray.updateFunc = [](ParticleData& particle, float deltaTime) {
			particle.transform.scale.y += 4.0f * deltaTime;
			particle.transform.scale.x -= 0.25f * deltaTime;
			if (particle.transform.scale.x < 0.0f) {
				particle.transform.scale.x = 0.0f;
			}
		};
		particleManager_->Emit("anchorHitRay", ray);
	}

	ParticleConfig flash{};
	flash.position = position;
	flash.scale = {0.9f, 0.9f, 1.0f};
	flash.color = {0.65f, 0.9f, 1.0f, 1.0f};
	flash.lifeTime = 0.18f;
	flash.updateFunc = [](ParticleData& particle, float deltaTime) {
		const float expansion = 4.0f * deltaTime;
		particle.transform.scale.x += expansion;
		particle.transform.scale.y += expansion;
	};
	particleManager_->Emit("anchorHitFlash", flash);
}

int BattleController::GetClearPulseIndex() const {
	if (!player_ || player_->GetClearPhase() != Player::ClearPhase::kLoop) {
		return -1;
	}
	// CalculateClearCameraRadius の寄りと同じ周期で数える
	const float rotation = player_->GetAnimationPlayLength(PlayerAnimator::State::kFlairing);
	const float cycle = clearCameraPulsesPerRotation_ > 0.0f ? rotation / clearCameraPulsesPerRotation_ : 0.0f;
	if (cycle <= 0.0f) {
		return -1;
	}
	const float time = player_->GetClearPhaseTimer() - clearCameraPulseOffset_;
	return time < 0.0f ? -1 : static_cast<int>(std::floor(time / cycle));
}

void BattleController::UpdateClearPulseEffects() {
	const int index = GetClearPulseIndex();
	if (index < 0) {
		lastClearPulseIndex_ = -1;
		return;
	}
	if (index != lastClearPulseIndex_) {
		lastClearPulseIndex_ = index;
		EmitClearBurst();
	}
}

void BattleController::EmitClearBurst() {
	if (!player_ || !particleManager_) {
		return;
	}
	const float maxTilt = (std::clamp)(clearBurstMaxTiltDeg_, 0.0f, 89.0f) * std::numbers::pi_v<float> / 180.0f;
	std::uniform_real_distribution<float> tiltDistribution(-maxTilt, maxTilt);
	std::uniform_real_distribution<float> rollDistribution(0.0f, std::numbers::pi_v<float> * 2.0f);

	// プレイヤーモデルの中心(腰)。少し手前に出して体に隠れないようにする
	Vector3 center = player_->GetModelCenterPosition();
	center.z -= 0.5f;
	for (int i = 0; i < clearBurstCount_; ++i) {
		const Vector3 tilt{ tiltDistribution(randomEngine_), tiltDistribution(randomEngine_), rollDistribution(randomEngine_) };
		EmitEnemyDefeatEffect(center, tilt);
	}
}

void BattleController::EmitEnemyDefeatEffect(const Vector3& position, const Vector3& tilt) {
	constexpr uint32_t kSparkCount = 16;

	// パーティクルの回転と同じ順(X->Y->Z, 行ベクトル)で方向を傾ける。
	// リングはビルボードで画面向きに裏返る(Y軸180度)ので、火花も同じく左右を反転して面を揃える
	const bool hasTilt = tilt.x != 0.0f || tilt.y != 0.0f || tilt.z != 0.0f;
	auto tiltDirection = [&tilt, hasTilt](float x, float y) {
		if (!hasTilt) {
			return Vector3{ x, y, 0.0f };
		}
		float z = 0.0f;
		// X
		float cx = std::cos(tilt.x), sx = std::sin(tilt.x);
		float y1 = y * cx - z * sx, z1 = y * sx + z * cx;
		// Y
		float cy = std::cos(tilt.y), sy = std::sin(tilt.y);
		float x2 = x * cy + z1 * sy, z2 = -x * sy + z1 * cy;
		// Z
		float cz = std::cos(tilt.z), sz = std::sin(tilt.z);
		float x3 = x2 * cz - y1 * sz, y3 = x2 * sz + y1 * cz;
		return Vector3{ -x3, y3, -z2 };
	};
	std::uniform_real_distribution<float> angleDistribution(
		0.0f, std::numbers::pi_v<float> * 2.0f);
	std::uniform_real_distribution<float> speedDistribution(3.0f, 6.0f);
	std::uniform_real_distribution<float> sizeDistribution(0.18f, 0.38f);

	for (uint32_t index = 0; index < kSparkCount; ++index) {
		const float angle = angleDistribution(randomEngine_);
		const float speed = speedDistribution(randomEngine_);

		ParticleConfig spark{};
		spark.position = position;
		const Vector3 direction = tiltDirection(std::cos(angle), std::sin(angle));
		spark.velocity = {
			direction.x * speed,
			direction.y * speed,
			0.0f // 奥行き方向は画面上ほぼ見えないので使わない
		};
		const float size = sizeDistribution(randomEngine_);
		spark.scale = {size, size, 1.0f};
		spark.color = {1.0f, 0.35f, 0.05f, 1.0f};
		spark.lifeTime = 0.55f;
		spark.updateFunc = [](ParticleData& particle, float deltaTime) {
			particle.transform.translate.x += particle.velocity.x * deltaTime;
			particle.transform.translate.y += particle.velocity.y * deltaTime;
			particle.velocity.y -= 5.0f * deltaTime;

			const float shrink = 0.35f * deltaTime;
			particle.transform.scale.x -= shrink;
			particle.transform.scale.y -= shrink;
			if (particle.transform.scale.x < 0.0f) {
				particle.transform.scale.x = 0.0f;
			}
			if (particle.transform.scale.y < 0.0f) {
				particle.transform.scale.y = 0.0f;
			}
		};
		particleManager_->Emit("enemyDefeatSpark", spark);
	}

	ParticleConfig ring{};
	ring.position = position;
	ring.rotate = tilt;
	ring.scale = {0.35f, 0.35f, 1.0f};
	ring.color = {1.0f, 0.65f, 0.1f, 1.0f};
	ring.lifeTime = 0.4f;
	ring.updateFunc = [](ParticleData& particle, float deltaTime) {
		const float expansion = 6.0f * deltaTime;
		particle.transform.scale.x += expansion;
		particle.transform.scale.y += expansion;
	};
	particleManager_->Emit("enemyDefeatRing", ring);
}

void BattleController::EmitLandingDustEffect(const Vector3& position) {
	constexpr uint32_t kDustCount = 10;
	std::uniform_real_distribution<float> horizontalSpeed(-3.2f, 3.2f);
	std::uniform_real_distribution<float> upwardSpeed(0.8f, 2.0f);
	std::uniform_real_distribution<float> sizeDistribution(0.25f, 0.55f);
	std::uniform_real_distribution<float> positionOffset(-0.5f, 0.5f);

	for (uint32_t index = 0; index < kDustCount; ++index) {
		ParticleConfig dust{};
		dust.position = {
			position.x + positionOffset(randomEngine_),
			position.y,
			position.z
		};
		dust.velocity = {
			horizontalSpeed(randomEngine_),
			upwardSpeed(randomEngine_),
			0.0f
		};
		const float size = sizeDistribution(randomEngine_);
		dust.scale = {size * 1.4f, size, 1.0f};
		dust.color = {0.55f, 0.42f, 0.25f, 0.7f};
		dust.lifeTime = 0.55f;
		dust.updateFunc = [](ParticleData& particle, float deltaTime) {
			particle.transform.translate.x += particle.velocity.x * deltaTime;
			particle.transform.translate.y += particle.velocity.y * deltaTime;
			particle.velocity.x *= 0.94f;
			particle.velocity.y -= 2.5f * deltaTime;

			particle.transform.scale.x += 0.7f * deltaTime;
			particle.transform.scale.y += 0.35f * deltaTime;
		};
		particleManager_->Emit("landingDust", dust);
	}
}

void BattleController::ResolveAnchorInstantLanding(Anchor& anchor) {
	Vector3 start = anchor.GetPosition();
	Vector3 velocity = anchor.GetVelocity();
	if (Length(velocity) <= 0.001f) {
		return;
	}
	Vector3 dir = Normalize(velocity);

	const float stepLen = 0.1f;
	const float maxDist = 50.0f;
	float currentDist = 0.0f;

	Vector3 currentPos = start;
	bool hit = false;

	while (currentDist < maxDist) {
		currentPos = Add(start, Multiply(currentDist, dir));

		IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(currentPos);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		if (type == MapChipType::kBlock) {
			// めり込み防止のためにアンカーの半径分（0.5f）＋マージン（0.05f）を逆方向に引き戻す
			const float pullBackDist = 0.55f;
			Vector3 hitPos = Subtract(currentPos, Multiply(pullBackDist, dir));
			anchor.SetPosition(hitPos);
			anchor.SetStandBy(true);
			hit = true;
			break;
		}

		AABB tempAABB;
		float w = 1.0f;
		float h = 1.0f;
		tempAABB.min = { currentPos.x - w / 2.0f, currentPos.y - h / 2.0f, -0.5f };
		tempAABB.max = { currentPos.x + w / 2.0f, currentPos.y + h / 2.0f, 0.5f };

		BaseEnemy* hitEnemy = nullptr;
		for (const auto& enemy : enemies_) {
			if (enemy->GetIsDead() || enemy->GetIsLockedOn()) {
				continue;
			}
			if (IsCollision(tempAABB, enemy->GetAABB())) {
				hitEnemy = enemy.get();
				break;
			}
		}

		if (hitEnemy) {
			anchor.SetPosition(currentPos);
			anchor.OnCollision();
			hitEnemy->OnAnchorHit();
			lockedOnEnemies_.push_back(hitEnemy);
			EmitAnchorHitEffect(currentPos);
			hit = true;
			break;
		}

		currentDist += stepLen;
	}

	if (!hit) {
		anchor.SetPosition(currentPos);
		anchor.SetDead(true);
	}

	anchor.SetInstantResolved(true);
}

void BattleController::StartClearPerformance() {
	SetHudVisible(false);
	hitStopTimer_ = 0.0f;
	if (player_) {
		player_->StartClearPerformance();
	}
}

bool BattleController::UpdateClearPerformance(float deltaTime) {
	if (!player_) {
		return true;
	}
	player_->UpdateClearPerformance();
	if (cameraController_) {
		cameraController_->Update(player_->GetWorldPosition(), deltaTime);
		cameraController_->SetCameraRadius(CalculateClearCameraRadius());
		UpdateClearPulseEffects(); // 寄るタイミングでパーティクル
	}
	UpdateVisuals(deltaTime);
	return player_->IsClearPerformanceFinished();
}

float BattleController::CalculateClearCameraRadius() const {
	using ClearPhase = Player::ClearPhase;
	const float defaultRadius = Camera::kDefaultRadius;
	const float timer = player_->GetClearPhaseTimer();

	switch (player_->GetClearPhase()) {
	case ClearPhase::kStart: {
		// Flair開始の長さをかけて、通常距離から寄りの距離へ (ease out)
		const float length = player_->GetAnimationPlayLength(PlayerAnimator::State::kFlairStart);
		float t = length > 0.0f ? (std::min)(timer / length, 1.0f) : 1.0f;
		t = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
		return defaultRadius + (clearCameraCloseRadius_ - defaultRadius) * t;
	}
	case ClearPhase::kLoop: {
		// 一定間隔で一瞬寄って戻る (0:寄りの距離 1:一番寄った距離)
		const float pulse = GetClearCameraPulse();
		return clearCameraCloseRadius_ + (clearCameraPulseRadius_ - clearCameraCloseRadius_) * pulse;
	}
	case ClearPhase::kFinish:
	case ClearPhase::kVanish:
	case ClearPhase::kDone:
		// Flair終了は何もしない (寄りの距離のまま)
		return clearCameraCloseRadius_;
	case ClearPhase::kLanding:
	case ClearPhase::kNone:
	default:
		return defaultRadius;
	}
}

float BattleController::GetClearCameraPulse() const {
	if (!player_ || player_->GetClearPhase() != Player::ClearPhase::kLoop) {
		return 0.0f;
	}
	// 間隔は 1回転の長さ / 1回転あたりの回数 で回転と同期させる
	const float rotation = player_->GetAnimationPlayLength(PlayerAnimator::State::kFlairing);
	const float cycle = clearCameraPulsesPerRotation_ > 0.0f ? rotation / clearCameraPulsesPerRotation_ : 0.0f;
	if (cycle <= 0.0f) {
		return 0.0f;
	}
	const float timer = player_->GetClearPhaseTimer();
	const float local = std::fmod(timer - clearCameraPulseOffset_ + cycle, cycle);
	if (local < clearCameraPulseInTime_) {
		return clearCameraPulseInTime_ > 0.0f ? local / clearCameraPulseInTime_ : 1.0f;
	}
	if (local < clearCameraPulseInTime_ + clearCameraPulseOutTime_) {
		const float t = clearCameraPulseOutTime_ > 0.0f ? (local - clearCameraPulseInTime_) / clearCameraPulseOutTime_ : 1.0f;
		return (1.0f - t) * (1.0f - t); // すっと戻る
	}
	return 0.0f;
}

void BattleController::UpdateVisuals(float deltaTime) {
	if (skyBox_) {
		skyBox_->Update(InitializeWorldTransform(), camera_);
	}
	UpdateGoal(deltaTime);
	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();
	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
			if (blockModels_[i][j]) {
				blockModels_[i][j]->Update(blockWorldTransforms_[i][j], camera_);
			}
		}
	}
	if (particleManager_) {
		particleManager_->Update(camera_);
	}
	GpuParticleManager::GetInstance()->Update(deltaTime, camera_);

	// 敵やプレイヤーの描画用モデル（カメラ追従WVP行列）を更新
	for (const auto& enemy : enemies_) {
		if (!enemy->GetIsDead()) {
			enemy->UpdateModel();
		}
	}
	if (player_) {
		player_->UpdateModel();
	}
	// ヒットストップ中もカメラに合わせて頭上表示の位置を追従させる
	if (anchorStockIndicator_ && player_) {
		anchorStockIndicator_->Update(player_.get(), camera_, deltaTime);
	}
}

void BattleController::TriggerGoSignal() {
	if (goSignalBanner_) {
		goSignalBanner_->Trigger(player_.get(), camera_);
	}
}

int BattleController::GetClearPulseTotal() const {
	if (!player_) {
		return 0;
	}
	// CalculateClearCameraRadius / GetClearPulseIndex と同じ周期で数える
	const float rotation = player_->GetAnimationPlayLength(PlayerAnimator::State::kFlairing);
	const float cycle = clearCameraPulsesPerRotation_ > 0.0f ? rotation / clearCameraPulsesPerRotation_ : 0.0f;
	if (cycle <= 0.0f) {
		return 0;
	}
	const float loopTime = rotation * static_cast<float>(player_->GetClearFlairLoopCount());
	const float span = loopTime - clearCameraPulseOffset_;
	if (span <= 0.0f) {
		return 0;
	}
	// ループ中に寄りが始まる回数 (誤差でちょうど割り切れる時に1回多く数えないよう少し引く)
	return (std::max)(1, static_cast<int>(std::ceil(span / cycle - 1e-4f)));
}

void BattleController::UpdateGoal(float deltaTime) {
	if (!goal_) {
		return;
	}
	// クリア演出の状態 (カメラの寄り・雷の色) をポータル演出に渡す
	goal_->SetClearPulse(GetClearCameraPulse());
	if (player_) {
		goal_->SetLightningBlend(player_->GetLightningColor(), GetGoalLightningBlend());
	}
	goal_->Update(deltaTime);
}

float BattleController::GetGoalLightningBlend() const {
	if (!player_) {
		return 0.0f;
	}
	switch (player_->GetClearPhase()) {
	case Player::ClearPhase::kLoop: {
		// n回目の寄りで 緑 -> 雷の色 へ n/合計 だけ近づける (最後の寄りでちょうど雷の色)
		const int index = GetClearPulseIndex();
		const int total = GetClearPulseTotal();
		if (index >= 0 && total > 0) {
			return (std::min)(static_cast<float>(index + 1) / static_cast<float>(total), 1.0f);
		}
		return 0.0f;
	}
	case Player::ClearPhase::kFinish:
	case Player::ClearPhase::kVanish:
	case Player::ClearPhase::kDone:
		return 1.0f; // 決めポーズから消えるまでは雷の色のまま
	default:
		return 0.0f; // 演出していない・着地待ち・Flair開始中はふだんの色
	}
}
