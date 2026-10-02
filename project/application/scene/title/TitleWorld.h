#pragma once
#include "SkyBox.h"
#include "Object3D.h"
#include "Lightning3D.h"
#include "Line3D.h"
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

        // エフェクト
        std::unique_ptr<Lightning3D> lightningEffect_ = nullptr;

        // 演出パラメータ
        float timer_ = 0.0f;
        float lightningTimer_ = 0.0f;
        int activeLightningNodeIndex_ = -1;

        // カメラ初期位置
        Vector3 initialCameraPos_{ 0.0f, 1.5f, -9.5f };

    private:
        void UpdateCamera(float deltaTime, bool isDiving, float diveProgress);
        void UpdateDataNodes(float deltaTime);
        void UpdateLightning(float deltaTime);
    };

}
