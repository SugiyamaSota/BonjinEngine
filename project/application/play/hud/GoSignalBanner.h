#pragma once
#include <memory>
#include "Sprite.h"
#include "TextSprite.h"
#include "Vector.h"

class Camera;
class Player;

namespace Bonjin {

/// <summary>
/// プレイヤー操作可能タイミング（戦闘開始）を告げる「GO!」演出バナー
/// 画面中央へのダイナミックズームイン、電撃パーティクル爆発、カメラシェイクを連動
/// </summary>
class GoSignalBanner {
public:
    GoSignalBanner();
    ~GoSignalBanner();

    /// <summary>
    /// 初期化（スプライト、テキストの生成）
    /// </summary>
    void Initialize();

    /// <summary>
    /// 「GO!」演出を発動
    /// </summary>
    /// <param name="player">パーティクル発生基準のプレイヤー</param>
    /// <param name="camera">シェイク対象のカメラ</param>
    void Trigger(Player* player, Camera* camera);

    /// <summary>
    /// 更新処理（アニメーション、タイマー進行、パーティクル契機判定）
    /// </summary>
    void Update(float deltaTime);

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// 現在演出中かどうか
    /// </summary>
    bool IsActive() const { return isActive_; }

private:
    std::unique_ptr<Sprite> bgBarSprite_;          // 背面のサイバーアクセント帯
    std::unique_ptr<Sprite> accentLineTopSprite_;  // 上部アクセントライン
    std::unique_ptr<Sprite> accentLineBtmSprite_;  // 下部アクセントライン
    std::unique_ptr<TextSprite> shadowTextSprite_; // ドロップシャドウ/グローテキスト
    std::unique_ptr<TextSprite> mainTextSprite_;   // メイン「GO!」テキスト

    bool isActive_ = false;
    float timer_ = 0.0f;
    const float kTotalDuration = 1.15f;
    const float kImpactTime = 0.22f;
    bool hasImpactTriggered_ = false;

    Player* cachedPlayer_ = nullptr;
    Camera* cachedCamera_ = nullptr;

    void OnImpact();
};

}
