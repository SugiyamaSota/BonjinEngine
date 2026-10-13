#include "TitleWorld.h"
#include "Camera.h"
#include "GpuParticleManager.h"
#include "SceneManager.h"
#include <cmath>
#include <random>
#include <algorithm>

#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif

using namespace Bonjin;

void TitleWorld::Initialize(Camera* camera) {
	camera_ = camera;
	timer_ = 0.0f;
	lightningTimer_ = 0.8f;
	activeLightningNodeIndex_ = -1;

	// カメラ初期設定
	initialCameraPos_ = { 0.0f, 1.5f, -9.5f };
	if (camera_) {
		camera_->SetTarget({ 0.0f, 1.0f, 0.0f });
		camera_->SetPosition(initialCameraPos_);
	}

	// 1. スカイボックス（暗黒・電脳空間）
	skyBox_ = std::make_unique<SkyBox>();
	skyBox_->CreateModel(ModelBuilder::ModelType::kSkyBox, "resources/textures/SkyBox_Dark.dds");

	// 2. 中央のアンカーコア（電脳世界の中心ノード）
	anchorCoreModel_ = std::make_unique<Object3D>();
	anchorCoreModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	anchorCoreModel_->SetColor({ 0.0f, 0.9f, 1.0f, 1.0f });
	anchorCoreModel_->SetEnableEnableEnvironmentMap(false);
	anchorCoreTransform_ = InitializeWorldTransform();
	anchorCoreTransform_.translate = { 0.0f, 1.0f, 0.0f };
	anchorCoreTransform_.scale = { 0.8f, 0.8f, 0.8f };

	// 3. 浮遊するデータノード群（キューブ）の生成
	dataNodes_.clear();
	std::mt19937 randEngine(42); // 固定シードで見栄えの良い配置を再現
	std::uniform_real_distribution<float> posXDist(-6.5f, 6.5f);
	std::uniform_real_distribution<float> posYDist(-2.0f, 4.0f);
	std::uniform_real_distribution<float> posZDist(-2.0f, 6.0f);
	std::uniform_real_distribution<float> scaleDist(0.35f, 0.75f);
	std::uniform_real_distribution<float> speedDist(0.8f, 2.0f);
	std::uniform_real_distribution<float> phaseDist(0.0f, 6.28f);

	const std::vector<Vector4> cyberColors = {
		{ 0.0f, 0.85f, 1.0f, 0.65f },  // ネオンシアン
		{ 0.1f, 0.35f, 0.95f, 0.60f }, // エレクトリックブルー
		{ 0.85f, 0.15f, 0.85f, 0.55f }, // ネオンマゼンタ
		{ 0.0f, 1.0f, 0.7f, 0.60f },   // サイバーグリーン
		{ 0.2f, 0.5f, 0.8f, 0.45f }    // ディープシアン
	};

	const int kNumNodes = 14;
	dataNodes_.reserve(kNumNodes);
	Object3D* cubeTemplate = nullptr;
	for (int i = 0; i < kNumNodes; ++i) {
		FloatingDataNode node;
		node.model = std::make_unique<Object3D>();
		if (!cubeTemplate) {
			node.model->CreateModel(ModelBuilder::ModelType::kCube, "resources/textures/cube.jpg");
			cubeTemplate = node.model.get();
		} else {
			node.model->CopyModelResourceFrom(*cubeTemplate, false);
		}
		node.model->SetEnableEnableEnvironmentMap(false);

		Vector3 basePos = { posXDist(randEngine), posYDist(randEngine), posZDist(randEngine) };
		// 中央のコアと重なりすぎないように離す
		if (std::abs(basePos.x) < 1.5f && std::abs(basePos.y - 1.0f) < 1.5f) {
			basePos.x += (basePos.x >= 0.0f ? 2.0f : -2.0f);
		}
		node.basePos = basePos;
		node.speed = speedDist(randEngine);
		node.phase = phaseDist(randEngine);
		node.color = cyberColors[i % cyberColors.size()];
		node.model->SetColor(node.color);

		node.transform = InitializeWorldTransform();
		float scale = scaleDist(randEngine);
		node.transform.scale = { scale, scale, scale };
		node.transform.translate = basePos;

		dataNodes_.push_back(std::move(node));
	}

	// 4. GPU雷霆エフェクトの初期設定
	lightningConfig_.coreColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	lightningConfig_.glowColor = { 0.2f, 0.85f, 1.0f, 1.0f };
	lightningConfig_.boltRadius = 2.0f;
	lightningConfig_.boltScale = 0.5f;
	lightningConfig_.subdivisions = 7;
	lightningConfig_.mainBranches = 3;
	lightningConfig_.sparksPerSegment = 2;
	lightningConfig_.impactSparks = 60;
	lightningConfig_.duration = 0.25f;
	lightningConfig_.intensity = 4.0f;
	lightningConfig_.sparkSpeed = 7.0f;
}

void TitleWorld::Unload() {
	skyBox_.reset();
	anchorCoreModel_.reset();
	dataNodes_.clear();
}

void TitleWorld::Update(float deltaTime, bool isDiving, float diveProgress) {
	timer_ += deltaTime;

	UpdateDataNodes(deltaTime);
	UpdateCamera(deltaTime, isDiving, diveProgress);
	UpdateLightning(deltaTime);
}

void TitleWorld::UpdateCamera(float deltaTime, bool isDiving, float diveProgress) {
	(void)deltaTime;
	if (!camera_) return;

	if (!isDiving) {
		
	} else {
		// ダイブ遷移中：カメラがアンカーコアに向かって急加速突入
		float eased = diveProgress * diveProgress * diveProgress; // EaseInCubic
		Vector3 targetPos = anchorCoreTransform_.translate;
		Vector3 diveCamPos;
		diveCamPos.x = initialCameraPos_.x + (targetPos.x - initialCameraPos_.x) * eased;
		diveCamPos.y = initialCameraPos_.y + (targetPos.y - initialCameraPos_.y) * eased;
		diveCamPos.z = initialCameraPos_.z + ((targetPos.z - 0.5f) - initialCameraPos_.z) * eased;
		camera_->SetPosition(diveCamPos);
		camera_->SetTarget(targetPos);
	}
}

void TitleWorld::UpdateDataNodes(float deltaTime) {
	(void)deltaTime;

	// スカイボックスの更新
	if (skyBox_) {
		skyBox_->Update(InitializeWorldTransform(), camera_);
	}

	// アンカーコアの更新（自転＆パルス拡縮）
	if (anchorCoreModel_) {
		anchorCoreTransform_.rotate.y += 0.015f;
		anchorCoreTransform_.rotate.x += 0.008f;
		float pulse = 0.8f + 0.06f * std::sin(timer_ * 3.5f);
		anchorCoreTransform_.scale = { pulse, pulse, pulse };
		anchorCoreModel_->Update(anchorCoreTransform_, camera_);
	}

	// 浮遊データノード群の更新
	for (auto& node : dataNodes_) {
		if (node.model) {
			node.transform.translate.y = node.basePos.y + std::sin(timer_ * node.speed + node.phase) * 0.45f;
			node.transform.translate.x = node.basePos.x + std::cos(timer_ * (node.speed * 0.7f) + node.phase) * 0.2f;
			node.transform.rotate.x += 0.012f;
			node.transform.rotate.y += 0.018f;
			node.model->Update(node.transform, camera_);
		}
	}
}

void TitleWorld::UpdateLightning(float deltaTime) {
	lightningTimer_ -= deltaTime;

	if (lightningNoiseTimer_ > 0.0f) {
		lightningNoiseTimer_ -= deltaTime;
		float ratio = (std::max)(0.0f, lightningNoiseTimer_ / lightningNoiseDuration_);
		// イージングをかけて瞬間的に光ってスッと消えるように減衰
		float currentNoise = baseNoiseAlpha_ + (lightningNoisePeakAlpha_ - baseNoiseAlpha_) * (ratio * ratio);
		SceneManager::GetInstance()->SetNoiseAlpha(currentNoise);
	} else {
		SceneManager::GetInstance()->SetNoiseAlpha(baseNoiseAlpha_);
	}


	// 一定間隔でランダムな浮遊ノードとアンカーコアの間にGPU雷霆を発生させる
	if (lightningTimer_ <= 0.0f) {
		if (!dataNodes_.empty()) {
			activeLightningNodeIndex_ = rand() % static_cast<int>(dataNodes_.size());
			float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
			lightningTimer_ = minLightningInterval_ + r * (maxLightningInterval_ - minLightningInterval_);

			const Vector3& nodePos = dataNodes_[activeLightningNodeIndex_].transform.translate;
			Vector3 corePos = anchorCoreTransform_.translate;

			GpuLightningConfig cfg = lightningConfig_;
			cfg.start = corePos;
			cfg.end = nodePos;
			if (useNodeColorForLightning_) {
				cfg.glowColor = dataNodes_[activeLightningNodeIndex_].color;
				cfg.glowColor.w = 1.0f;
			}

			// --- 設定したコンフィグでGPU雷霆を発生 ---
			GpuParticleManager::GetInstance()->EmitLightning(cfg);

			// --- 雷発生時にノイズをトリガー ---
			if (enableLightningNoise_) {
				lightningNoiseTimer_ = lightningNoiseDuration_;
			}
		}
	}
}

void TitleWorld::Draw() {
	if (skyBox_) {
		skyBox_->Draw();
	}
	for (auto& node : dataNodes_) {
		if (node.model) {
			node.model->Draw();
		}
	}
	if (anchorCoreModel_) {
		anchorCoreModel_->Draw();
	}
}

void TitleWorld::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Title World & Background Lightning")) {
		if (ImGui::Button("Trigger Lightning Now")) {
			if (!dataNodes_.empty()) {
				activeLightningNodeIndex_ = rand() % static_cast<int>(dataNodes_.size());
				const Vector3& nodePos = dataNodes_[activeLightningNodeIndex_].transform.translate;
				Vector3 corePos = anchorCoreTransform_.translate;

				GpuLightningConfig cfg = lightningConfig_;
				cfg.start = corePos;
				cfg.end = nodePos;
				if (useNodeColorForLightning_) {
					cfg.glowColor = dataNodes_[activeLightningNodeIndex_].color;
					cfg.glowColor.w = 1.0f;
				}
				GpuParticleManager::GetInstance()->EmitLightning(cfg);
			}
		}

		ImGui::Checkbox("Use Node Color For Glow", &useNodeColorForLightning_);
		ImGui::DragFloatRange2("Interval Range", &minLightningInterval_, &maxLightningInterval_, 0.05f, 0.1f, 10.0f, "Min: %.2fs", "Max: %.2fs");
		ImGui::ColorEdit4("Default Core Color", &lightningConfig_.coreColor.x);
		ImGui::ColorEdit4("Default Glow Color", &lightningConfig_.glowColor.x);
		ImGui::DragFloat("Bolt Radius (Jitter)", &lightningConfig_.boltRadius, 0.01f, 0.0f, 2.0f, "%.2f");
		ImGui::DragFloat("Bolt Scale (Thickness)", &lightningConfig_.boltScale, 0.01f, 0.01f, 1.0f, "%.2f");
		ImGui::SliderInt("Subdivisions", &lightningConfig_.subdivisions, 2, 9);
		ImGui::SliderInt("Main Branches", &lightningConfig_.mainBranches, 0, 8);
		ImGui::SliderInt("Sparks Per Segment", &lightningConfig_.sparksPerSegment, 0, 5);
		ImGui::SliderInt("Impact Sparks", &lightningConfig_.impactSparks, 0, 200);
		ImGui::DragFloat("Duration (s)", &lightningConfig_.duration, 0.01f, 0.05f, 1.0f, "%.2fs");
		ImGui::DragFloat("Intensity (Bloom)", &lightningConfig_.intensity, 0.1f, 0.5f, 15.0f, "%.1f");
		ImGui::DragFloat("Spark Speed", &lightningConfig_.sparkSpeed, 0.2f, 1.0f, 30.0f, "%.1f");

		ImGui::TreePop();
	}
#endif
}

