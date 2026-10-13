#pragma once
#include "SkyBox.h"
#include "Object3D.h"
#include "GpuParticleStructs.h"
#include "Convert.h"
#include <memory>
#include <vector>

class Camera;

namespace Bonjin
{

    struct FloatingDataNode {
        std::unique_ptr<Object3D> model;
        WorldTransform transform{};
        Vector3 basePos{ 0.0f, 0.0f, 0.0f };
        float speed = 1.0f;
        float phase = 0.0f;
        Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    /// <summary>
    /// タイトル画面の3D電脳空間・背景演出・カメラ制御を管理するクラス
    /// </summary>
    class TitleWorld
    {
    public:
        TitleWorld() = default;
        ~TitleWorld() = default;

        /// <summary>
        /// 初期化
        /// </summary>
        void Initialize(Camera* camera);

        /// <summary>
        /// 解放
        /// </summary>
        void Unload();

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <param name="deltaTime">経過時間</param>
        /// <param name="isDiving">ダイブ中（フェードアウト遷移中）フラグ</param>
        /// <param name="diveProgress">ダイブ進行度 (0.0f ~ 1.0f)</param>
        void Update(float deltaTime, bool isDiving = false, float diveProgress = 0.0f);

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw();

        /// <summary>
        /// ImGui描画
        /// </summary>
        void DrawImGui();

        /// <summary>
        /// アンカーコアの座標を取得
        /// </summary>
        const Vector3& GetCorePosition() const { return anchorCoreTransform_.translate; }

    private:
        Camera* camera_ = nullptr;

        // 3D背景・モデル
        std::unique_ptr<SkyBox> skyBox_ = nullptr;
        std::unique_ptr<Object3D> anchorCoreModel_ = nullptr;
        WorldTransform anchorCoreTransform_{};
        std::vector<FloatingDataNode> dataNodes_;

        // GPU雷霆演出設定
        GpuLightningConfig lightningConfig_{};
        float minLightningInterval_ = 0.8f;
        float maxLightningInterval_ = 2.0f;
        bool useNodeColorForLightning_ = true;

        // 演出パラメータ
        float timer_ = 0.0f;
        float lightningTimer_ = 0.0f;
        int activeLightningNodeIndex_ = -1;

        bool enableLightningNoise_ = true;
        float lightningNoiseTimer_ = 0.0f;
        float lightningNoiseDuration_ = 0.4f;   // ノイズの持続時間（秒）
        float lightningNoisePeakAlpha_ = 0.5f;  // 雷発生瞬間の最大ノイズ強度 (0.0f ~ 1.0f)
        float baseNoiseAlpha_ = 0.0f;         // 平常時の微小ノイズ強度

        // カメラ初期位置
        Vector3 initialCameraPos_{ 0.0f, 1.5f, -9.5f };

    private:
        void UpdateCamera(float deltaTime, bool isDiving, float diveProgress);
        void UpdateDataNodes(float deltaTime);
        void UpdateLightning(float deltaTime);
    };

}
