#define NOMINMAX
#include "GoSignalBanner.h"
#include "../gameObject/player/Player.h"
#include "Camera.h"
#include "GpuParticleManager.h"
#include "BonjinEngine.h"
#include "SePlayer.h"
#include "Matrix.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Bonjin {

GoSignalBanner::GoSignalBanner() = default;
GoSignalBanner::~GoSignalBanner() = default;

void GoSignalBanner::Initialize() {
    // 1. 中央の半透明ダークサイバー帯
    bgBarSprite_ = std::make_unique<Sprite>();
    bgBarSprite_->Initialize("default.png");
    bgBarSprite_->SetAnchor({ 0.5f, 0.5f, 0.0f });
    bgBarSprite_->SetSize({ 1280.0f, 150.0f });
    bgBarSprite_->SetTranslate({ 640.0f, 360.0f });
    bgBarSprite_->SetColor({ 0.01f, 0.03f, 0.08f, 0.0f });

    // 2. 上部サイバー発光スリットライン
    accentLineTopSprite_ = std::make_unique<Sprite>();
    accentLineTopSprite_->Initialize("default.png");
    accentLineTopSprite_->SetAnchor({ 0.5f, 0.5f, 0.0f });
    accentLineTopSprite_->SetSize({ 1280.0f, 3.0f });
    accentLineTopSprite_->SetTranslate({ 640.0f, 285.0f });
    accentLineTopSprite_->SetColor({ 0.0f, 0.85f, 1.0f, 0.0f });

    // 3. 下部サイバー発光スリットライン
    accentLineBtmSprite_ = std::make_unique<Sprite>();
    accentLineBtmSprite_->Initialize("default.png");
    accentLineBtmSprite_->SetAnchor({ 0.5f, 0.5f, 0.0f });
    accentLineBtmSprite_->SetSize({ 1280.0f, 3.0f });
    accentLineBtmSprite_->SetTranslate({ 640.0f, 435.0f });
    accentLineBtmSprite_->SetColor({ 0.0f, 0.85f, 1.0f, 0.0f });

    // 4. シャドウ・グローテキスト（白ベーステクスチャ生成、マテリアルでネオンシアン）
    shadowTextSprite_ = std::make_unique<TextSprite>();
    shadowTextSprite_->Initialize();
    shadowTextSprite_->SetAnchor({ 0.5f, 0.5f, 0.0f });
    shadowTextSprite_->SetText(L"GO!", 140, RGB(255, 255, 255));
    shadowTextSprite_->SetTranslate({ 643.0f, 363.0f });
    shadowTextSprite_->SetColor({ 0.05f, 0.88f, 1.0f, 0.0f });

    // 5. メイン「GO!」テキスト（白ベーステクスチャ生成、マテリアルでエレクトリックゴールド）
    mainTextSprite_ = std::make_unique<TextSprite>();
    mainTextSprite_->Initialize();
    mainTextSprite_->SetAnchor({ 0.5f, 0.5f, 0.0f });
    mainTextSprite_->SetText(L"GO!", 140, RGB(255, 255, 255));
    mainTextSprite_->SetTranslate({ 640.0f, 360.0f });
    mainTextSprite_->SetColor({ 1.0f, 0.95f, 0.35f, 0.0f });

    isActive_ = false;
    timer_ = 0.0f;
    hasImpactTriggered_ = false;
}

void GoSignalBanner::Trigger(Player* player, Camera* camera) {
    isActive_ = true;
    timer_ = 0.0f;
    hasImpactTriggered_ = false;
    cachedPlayer_ = player;
    cachedCamera_ = camera;
}

void GoSignalBanner::Update(float deltaTime) {
    if (!isActive_) {
        return;
    }

    timer_ += deltaTime;

    // --- アニメーション状態計算 ---
    float currentScale = 1.0f;
    float currentAlpha = 0.0f;
    float bgAlpha = 0.0f;

    if (timer_ < kImpactTime) {
        // [フェーズ1: 急速ズームイン突撃] (3.5倍から1.0倍へ急激に減速突入)
        float p = std::clamp(timer_ / kImpactTime, 0.0f, 1.0f);
        float invP = 1.0f - p;
        currentScale = 1.0f + 2.5f * (invP * invP);
        currentAlpha = std::clamp(p * 2.5f, 0.0f, 1.0f);
        bgAlpha = p * 0.75f;
    } else {
        // [着地インパクトの瞬間！]
        if (!hasImpactTriggered_) {
            OnImpact();
            hasImpactTriggered_ = true;
        }

        if (timer_ < 0.75f) {
            // [フェーズ2: インパクト後の強調・鼓動]
            float aliveTime = timer_ - kImpactTime;
            float pulse = std::sin(aliveTime * 14.0f) * 0.04f;
            currentScale = 1.0f + (std::max)(0.0f, pulse);
            currentAlpha = 1.0f;
            bgAlpha = 0.75f;
        } else if (timer_ < kTotalDuration) {
            // [フェーズ3: 前方へ突き抜ける光速フェードアウト]
            float p = (timer_ - 0.75f) / (kTotalDuration - 0.75f);
            p = std::clamp(p, 0.0f, 1.0f);
            // 1.0倍から1.7倍へ拡大しながら消滅
            currentScale = 1.0f + 0.7f * (p * p);
            currentAlpha = 1.0f - p;
            bgAlpha = 0.75f * (1.0f - p);
        } else {
            // [終了]
            isActive_ = false;
            return;
        }
    }

    // スプライト変形の適用
    Vector2 scaleVec = { currentScale, currentScale };

    // 背景バー
    bgBarSprite_->SetColor({ 0.01f, 0.03f, 0.08f, bgAlpha * 0.75f });
    bgBarSprite_->Update();

    // 上下スリットライン
    accentLineTopSprite_->SetColor({ 0.0f, 0.85f, 1.0f, bgAlpha });
    accentLineTopSprite_->Update();

    accentLineBtmSprite_->SetColor({ 0.0f, 0.85f, 1.0f, bgAlpha });
    accentLineBtmSprite_->Update();

    // シャドウテキスト
    shadowTextSprite_->SetScale(scaleVec);
    shadowTextSprite_->SetColor({ 0.05f, 0.88f, 1.0f, currentAlpha * 0.85f });
    shadowTextSprite_->Update();

    // メインテキスト
    mainTextSprite_->SetScale(scaleVec);
    mainTextSprite_->SetColor({ 1.0f, 0.95f, 0.35f, currentAlpha });
    mainTextSprite_->Update();
}

void GoSignalBanner::OnImpact() {
    // 1. 画面中央「GO!」文字の背後・周囲から放射状に飛び散るGPUパーティクル演出！
    if (cachedCamera_) {
        // カメラのビュー行列逆行列から、カメラ姿勢（位置・右・上・前）を取得
        Matrix4x4 camWorld = Inverse(cachedCamera_->GetViewMatrix());
        Vector3 camRight = { camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2] };
        Vector3 camUp = { camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2] };
        Vector3 camForward = { camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2] };
        Vector3 camPos = { camWorld.m[3][0], camWorld.m[3][1], camWorld.m[3][2] };

        // 画面中央・文字背後の3D空間位置（カメラ前方近傍）
        float focusDistance = 5.5f;
        Vector3 burstCenter = Add(camPos, Multiply(focusDistance, camForward));

        // (A) 中心フラッシュ閃光（文字着地瞬間のパッとした輝き）
        GpuParticleManager::GetInstance()->EmitFlashGlow(
            burstCenter,
            Vector4(1.0f, 1.0f, 1.0f, 1.0f),
            1.6f,   // スケール
            0.28f,  // 寿命
            4.0f    // 輝度
        );

        // (B) 画面中央「GO!」文字の背後から全方位へ炸裂するエレクトリックゴールドスパーク！
        GpuParticleManager::GetInstance()->EmitRadialSparks(
            burstCenter,
            75,                                  // 個数
            Vector4(1.0f, 0.92f, 0.25f, 1.0f),  // エレクトリックゴールド
            camRight, camUp,
            12.0f,                               // 初速
            0.65f,                               // 寿命
            0.22f                                // スケール
        );

        // (C) 画面中央から放射状に飛び散るネオンシアンスパーク！
        GpuParticleManager::GetInstance()->EmitRadialSparks(
            burstCenter,
            55,                                  // 個数
            Vector4(0.1f, 0.88f, 1.0f, 1.0f),   // ネオンシアン
            camRight, camUp,
            15.5f,                               // 高速放射
            0.50f,                               // 寿命
            0.16f                                // スケール
        );

        // (D) コアの閃光ホワイトスパーク
        GpuParticleManager::GetInstance()->EmitRadialSparks(
            burstCenter,
            30,                                  // 個数
            Vector4(1.0f, 1.0f, 1.0f, 1.0f),   // ホワイト
            camRight, camUp,
            7.5f,                                // 初速
            0.38f,                               // 寿命
            0.26f                                // スケール
        );

        // (E) 文字背後から四方八方にバリバリッと走る雷光（Radial Lightning）
        for (int i = 0; i < 6; ++i) {
            float baseAngle = (i * 60.0f + 15.0f) * (std::numbers::pi_v<float> / 180.0f);
            Vector3 boltDir = Add(Multiply(std::cos(baseAngle), camRight), Multiply(std::sin(baseAngle), camUp));
            Vector3 endPos = Add(burstCenter, Multiply(4.2f, boltDir));

            GpuLightningConfig lightningCfg{};
            lightningCfg.start = burstCenter;
            lightningCfg.end = endPos;
            lightningCfg.coreColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            lightningCfg.glowColor = { 0.2f, 0.9f, 1.0f, 1.0f };
            lightningCfg.boltScale = 0.28f;
            lightningCfg.duration = 0.26f;
            lightningCfg.intensity = 2.8f;
            lightningCfg.subdivisions = 3;
            lightningCfg.mainBranches = 1;
            lightningCfg.impactSparks = 10;
            GpuParticleManager::GetInstance()->EmitLightning(lightningCfg);
        }

        // (F) インパクト瞬間のカメラシェイク（ズドンと心地よい手応え）
        cachedCamera_->StartShake(3.0f, 0.22f);
    }

    // 2. プレイヤー周囲にも電撃とスパークを連動（キャラクターと演出の一体感）
    if (cachedPlayer_) {
        Vector3 playerPos = cachedPlayer_->GetWorldPosition();

        GpuParticleManager::GetInstance()->EmitSparks(
            playerPos,
            40,
            Vector4(1.0f, 0.92f, 0.25f, 1.0f),
            10.0f,
            0.55f,
            0.20f
        );

        GpuLightningConfig lightningCfg{};
        lightningCfg.start = { playerPos.x + 0.3f, playerPos.y + 10.0f, playerPos.z };
        lightningCfg.end = playerPos;
        lightningCfg.coreColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightningCfg.glowColor = { 0.2f, 0.9f, 1.0f, 1.0f };
        lightningCfg.boltScale = 0.32f;
        lightningCfg.duration = 0.26f;
        lightningCfg.intensity = 2.5f;
        lightningCfg.subdivisions = 4;
        lightningCfg.mainBranches = 1;
        lightningCfg.impactSparks = 15;
        GpuParticleManager::GetInstance()->EmitLightning(lightningCfg);
    }

    // 3. サウンド再生（test1.mp3）
    SePlayer::GetInstance()->Play("resources/sounds/se/test1.mp3", 0.8f, 1.0f);
}

void GoSignalBanner::Draw() {
    if (!isActive_) {
        return;
    }

    bgBarSprite_->Draw();
    accentLineTopSprite_->Draw();
    accentLineBtmSprite_->Draw();
    shadowTextSprite_->Draw();
    mainTextSprite_->Draw();
}

}
