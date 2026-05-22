#pragma once
#include "../bonjin/BonjinEngine.h" // 既存のエンジンヘッダをインクルード
#include <vector>

namespace BonjinEngine {

    // グリッド描画用の頂点構造体
    struct GridVertex {
        Vector4 position;
    };

    // ワールドビュープロジェクション行列用の定数バッファ
    struct GridTransformationMatrix {
        Matrix4x4 worldviewProjection;
    };

    // 新しいPlane構造体 (BonjinEngine内に定義されているか、別途定義が必要です)
    // ここでは、Grid.h内で直接定義します
    struct Plane {
        Vector3 normal;
        float distance;
    };

    // 平面を正規化するヘルパー関数の前方宣言
    // これがBonjinEngineの共通ユーティリティにない場合、Grid.cpp内で定義します
    void NormalizePlane(Plane& plane);


    class Grid {
    public:
        /// <summary>
        /// 初期化
        /// </summary>
        /// <param name="gridSize">グリッドの範囲 (XとZ軸の最大値)</param>
        /// <param name="numDivisions">グリッドの分割数</param>
        void Initialize(float gridSize = 300.0f, int numDivisions = 300);

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <param name="camera">現在のカメラ</param>
        void Update(Camera* camera);

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Grid();

    private:
        // グリッドの頂点データ
        std::vector<GridVertex> gridVertices_;
        // グリッド用頂点バッファ
        Microsoft::WRL::ComPtr<ID3D12Resource> gridVertexBuffer_;
        D3D12_VERTEX_BUFFER_VIEW gridVbView_{};

        // グリッド用定数バッファ
        Microsoft::WRL::ComPtr<ID3D12Resource> gridTransformationMatrixResource_;
        GridTransformationMatrix* gridTransformationMatrixData_ = nullptr;

        // グリッドのサイズと分割数
        float gridSize_;
        int numDivisions_;

        // 現在のカメラの視錐台（フラスタム）の情報を保持する構造体
        struct Frustum {
            Plane planes[6]; // 視錐台の6つの平面 (右, 左, 下, 上, 近, 遠)
        };

        Frustum currentFrustum_;

         // グリッドのワールド変換行列 (追加)
        Matrix4x4 worldMatrix_; 

        /// <summary>
        /// 現在のカメラ位置に基づいてグリッドの中心座標を計算し、頂点バッファを更新する
        /// </summary>
        /// <param name="cameraPosition">カメラのワールド座標</param>
        void GenerateCenteredGridVertices(const Vector3& cameraPosition);

        /// <summary>
        /// 視錐台の平面を計算する
        /// </summary>
        /// <param name="viewProjectionMatrix">ビュープロジェクション行列</param>
        void CalculateFrustumPlanes(const Matrix4x4& viewProjectionMatrix);

        /// <summary>
        /// 指定された範囲内のグリッド線を生成し、頂点バッファを更新する
        /// </summary>
        /// <param name="minX">描画するXの最小範囲</param>
        /// <param name="maxX">描画するXの最大範囲</param>
        /// <param name="minZ">描画するZの最小範囲</param>
        /// <param name="maxZ">描画するZの最大範囲</param>
        void GenerateVisibleGridVertices(float minX, float maxX, float minZ, float maxZ);

        /// <summary>
        /// 軸並行バウンディングボックス (AABB) が視錐台と交差するか判定する
        /// </summary>
        /// <param name="minBounds">AABBの最小点</param>
        /// <param name="maxBounds">AABBの最大点</param>
        /// <returns>交差していればtrue、そうでなければfalse</returns>
        bool IsAABBInFrustum(const Vector3& minBounds, const Vector3& maxBounds);

        /// <summary>
        /// 点が視錐台内にあるか判定する
        /// </summary>
        /// <param name="point">判定する点</param>
        /// <returns>点が含まれていればtrue、そうでなければfalse</returns>
        bool IsPointInFrustum(const Vector3& point);

        void NormalizePlane(Plane& plane) {
            // 平面の法線ベクトルの長さを計算
            float length = Length(plane.normal); // Vector3のLength関数を使用

            // 長さが0でなければ正規化
            if (length != 0.0f) {
                plane.normal.x = plane.normal.x / length;
                plane.normal.y = plane.normal.y / length;
                plane.normal.z = plane.normal.z / length;
                plane.distance /= length; // distanceも同じ係数で割る
            }
        }
    };
} // namespace BonjinEngine