#include "Dive.h"
#include "../TitleScene.h"

#include "SceneManager.h"

#include <algorithm>

namespace Bonjin::TitlePhase {

Dive::Dive(const std::string& targetSceneType)
	: targetSceneType_(targetSceneType), timer_(0.0f) {
}

void Dive::Enter(TitleScene* scene) {
	timer_ = 0.0f;

	// ラジアルブラーの追加（光速突入演出）
	auto sm = SceneManager::GetInstance();
	auto activeEffects = sm->GetActiveEffects();
	if (std::find(activeEffects.begin(), activeEffects.end(), PostEffect::kRadialBlur) == activeEffects.end()) {
		sm->AddPostEffect(PostEffect::kRadialBlur);
	}

	// スタート時のカメラ激震シェイク
	if (scene->GetCamera()) {
		scene->GetCamera()->StartShake(4.5f, 1.0f);
	}
}

void Dive::Update(TitleScene* scene, float deltaTime) {
	timer_ += deltaTime;
	float progress = std::clamp(timer_ / kDiveDuration, 0.0f, 1.0f);
	float eased = progress * progress * progress;

	// ラジアルブラーを加速強化（光速の吸引感）
	auto sm = SceneManager::GetInstance();
	sm->SetRadialBlurWidth(eased * 0.11f);

	// ズーム最高潮に達した瞬間、共通トランジション（CRT電源断 → 静寂ブラックアウト → 次シーンディゾルブ）を起動！
	if (progress >= 1.0f && !transitionRequested_) {
		transitionRequested_ = true;
		// BGM停止とSE再生
		BgmPlayer::GetInstance()->Stop();
		sm->SetRadialBlurWidth(0.0f);
		sm->RemovePostEffect(PostEffect::kRadialBlur);

		uint32_t seId = scene->GetShutdownSEId();
		if (!SePlayer::GetInstance()->IsPlaying(seId)) {
			SePlayer::GetInstance()->Play(seId, 1.0f);
		}

		// シーン遷移を要求
		sm->RequestSceneChange(targetSceneType_);
	}
}

void Dive::Exit(TitleScene* scene) {
	(void)scene;
	auto sm = SceneManager::GetInstance();
	sm->SetRadialBlurWidth(0.0f);
	sm->RemovePostEffect(PostEffect::kRadialBlur);
}

float Dive::GetDiveProgress() const {
	return std::clamp(timer_ / kDiveDuration, 0.0f, 1.0f);
}

}
