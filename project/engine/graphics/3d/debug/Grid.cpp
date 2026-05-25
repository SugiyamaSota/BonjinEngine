#include "Grid.h"
#include <algorithm> // std::min, std::max のために必要

namespace BonjinEngine {

    void Grid::Initialize(float gridSize, int numDivisions) {
        gridSize_ = gridSize;
        numDivisions_ = numDivisions;

        // グリッド用定数バッファの作成
        Microsoft::WRL::ComPtr<ID3D12Device> device = DirectXCommon::GetInstance()->GetDevice();
        gridTransformationMatrixResource_ = CreateBufferResource(device.Get(), sizeof(GridTransformationMatrix));
        HRESULT hr = gridTransformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&gridTransformationMatrixData_));
        assert(SUCCEEDED(hr));

        // 初期状態で一度、全体のグリッドを生成して頂点バッファを作成
        // Updateで視錐台に応じた範囲を再生成・更新するため、ここでは仮の範囲でOK
        GenerateVisibleGridVertices(-gridSize_ / 2.0f, gridSize_ / 2.0f, -gridSize_ / 2.0f, gridSize_ / 2.0f);

        worldMatrix_ = MakeIdentity4x4();
    }

    void Grid::Update(Camera* camera) {
        // カメラのビュープロジェクション行列を定数バッファに設定
        // グリッド自体のワールド変換は、個々の頂点座標の生成で対応するため、ここでは単位行列とみなす
        gridTransformationMatrixData_->worldviewProjection = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());

        // グリッドの中心をカメラのXZ平面に合わせる
        Vector3 cameraPosition = camera->GetEye();

        // グリッドのX, Z方向の中心を、グリッドのステップサイズの倍数に丸めることで、
        // グリッド線が常に整列するようにする
        float step = gridSize_ / static_cast<float>(numDivisions_);
        Vector3 alignedCameraPosition = {
            std::round(cameraPosition.x / step) * step,
            0.0f, // グリッドはY=0平面にあるため、Y座標は固定
            std::round(cameraPosition.z / step) * step
        };

        // カメラ位置に基づいてグリッド頂点を再生成
        GenerateCenteredGridVertices(alignedCameraPosition);
    }

    void Grid::Draw() {
        if (gridVertices_.empty() || !gridVertexBuffer_) {
            return; // 頂点がない場合は描画しない
        }

        ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();
        const auto& pso = DirectXCommon::GetInstance()->GetPSO();

        //commandList->SetGraphicsRootSignature(DirectXCommon::GetInstance()->GetPSO()->GetRootSignature(PrimitiveType::kGrid));
        //commandList->SetPipelineState(DirectXCommon::GetInstance()->GetPSO()->GetPipelineState(PrimitiveType::kGrid, BlendMode::kNone));
        commandList->IASetVertexBuffers(0, 1, &gridVbView_);
        commandList->SetGraphicsRootConstantBufferView(0, gridTransformationMatrixResource_->GetGPUVirtualAddress()); // b0 レジスタに設定
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST); // ラインリストとして描画
        commandList->DrawInstanced(static_cast<UINT>(gridVertices_.size()), 1, 0, 0);
    }

    Grid::~Grid() {
        if (gridTransformationMatrixData_) {
            gridTransformationMatrixResource_->Unmap(0, nullptr);
            gridTransformationMatrixData_ = nullptr;
        }
        // gridVertexBuffer_はComPtrが管理するため、明示的なReleaseは不要
    }

    void Grid::CalculateFrustumPlanes(const Matrix4x4& viewProjectionMatrix) {
        // 視錐台の6つの平面を計算します。
        // これらの平面は、ビュープロジェクション行列から導出されます。

        // 右平面
        currentFrustum_.planes[0].normal.x = viewProjectionMatrix.m[0][3] - viewProjectionMatrix.m[0][0];
        currentFrustum_.planes[0].normal.y = viewProjectionMatrix.m[1][3] - viewProjectionMatrix.m[1][0];
        currentFrustum_.planes[0].normal.z = viewProjectionMatrix.m[2][3] - viewProjectionMatrix.m[2][0];
        currentFrustum_.planes[0].distance = viewProjectionMatrix.m[3][3] - viewProjectionMatrix.m[3][0];
        NormalizePlane(currentFrustum_.planes[0]);

        // 左平面
        currentFrustum_.planes[1].normal.x = viewProjectionMatrix.m[0][3] + viewProjectionMatrix.m[0][0];
        currentFrustum_.planes[1].normal.y = viewProjectionMatrix.m[1][3] + viewProjectionMatrix.m[1][0];
        currentFrustum_.planes[1].normal.z = viewProjectionMatrix.m[2][3] + viewProjectionMatrix.m[2][0];
        currentFrustum_.planes[1].distance = viewProjectionMatrix.m[3][3] + viewProjectionMatrix.m[3][0];
        NormalizePlane(currentFrustum_.planes[1]);

        // 下平面
        currentFrustum_.planes[2].normal.x = viewProjectionMatrix.m[0][3] + viewProjectionMatrix.m[0][1];
        currentFrustum_.planes[2].normal.y = viewProjectionMatrix.m[1][3] + viewProjectionMatrix.m[1][1];
        currentFrustum_.planes[2].normal.z = viewProjectionMatrix.m[2][3] + viewProjectionMatrix.m[2][1];
        currentFrustum_.planes[2].distance = viewProjectionMatrix.m[3][3] + viewProjectionMatrix.m[3][1];
        NormalizePlane(currentFrustum_.planes[2]);

        // 上平面
        currentFrustum_.planes[3].normal.x = viewProjectionMatrix.m[0][3] - viewProjectionMatrix.m[0][1];
        currentFrustum_.planes[3].normal.y = viewProjectionMatrix.m[1][3] - viewProjectionMatrix.m[1][1];
        currentFrustum_.planes[3].normal.z = viewProjectionMatrix.m[2][3] - viewProjectionMatrix.m[2][1];
        currentFrustum_.planes[3].distance = viewProjectionMatrix.m[3][3] - viewProjectionMatrix.m[3][1];
        NormalizePlane(currentFrustum_.planes[3]);

        // 近平面
        currentFrustum_.planes[4].normal.x = viewProjectionMatrix.m[0][2];
        currentFrustum_.planes[4].normal.y = viewProjectionMatrix.m[1][2];
        currentFrustum_.planes[4].normal.z = viewProjectionMatrix.m[2][2];
        currentFrustum_.planes[4].distance = viewProjectionMatrix.m[3][2];
        NormalizePlane(currentFrustum_.planes[4]);

        // 遠平面
        currentFrustum_.planes[5].normal.x = viewProjectionMatrix.m[0][3] - viewProjectionMatrix.m[0][2];
        currentFrustum_.planes[5].normal.y = viewProjectionMatrix.m[1][3] - viewProjectionMatrix.m[1][2];
        currentFrustum_.planes[5].normal.z = viewProjectionMatrix.m[2][3] - viewProjectionMatrix.m[2][2];
        currentFrustum_.planes[5].distance = viewProjectionMatrix.m[3][3] - viewProjectionMatrix.m[3][2];
        NormalizePlane(currentFrustum_.planes[5]);
    }

    void Grid::GenerateVisibleGridVertices(float minX, float maxX, float minZ, float maxZ) {
        gridVertices_.clear();
        const float step = gridSize_ / static_cast<float>(numDivisions_);
        const float halfGridSize = gridSize_ / 2.0f;

        // X軸に平行な線
        for (int i = 0; i <= numDivisions_; ++i) {
            float z = -halfGridSize + i * step;
            // 描画範囲内にZ座標があるかチェック
            if (z >= minZ && z <= maxZ) {
                Vector3 p1_candidate = { max(minX, -halfGridSize), 0.0f, z };
                Vector3 p2_candidate = { min(maxX, halfGridSize), 0.0f, z };

                // 線分全体が完全に視錐台の外にある場合はスキップ
                // ここでは、線分の両端が視錐台内にあるか、線分を囲むAABBが視錐台と交差するかをチェック
                if (IsPointInFrustum(p1_candidate) || IsPointInFrustum(p2_candidate) || IsAABBInFrustum({ p1_candidate.x, -0.1f, p1_candidate.z }, { p2_candidate.x, 0.1f, p2_candidate.z })) {
                    gridVertices_.push_back({ {p1_candidate.x, p1_candidate.y, p1_candidate.z, 1.0f} });
                    gridVertices_.push_back({ {p2_candidate.x, p2_candidate.y, p2_candidate.z, 1.0f} });
                }
            }
        }
        // Z軸に平行な線
        for (int i = 0; i <= numDivisions_; ++i) {
            float x = -halfGridSize + i * step;
            // 描画範囲内にX座標があるかチェック
            if (x >= minX && x <= maxX) {
                Vector3 p1_candidate = { x, 0.0f, max(minZ, -halfGridSize) };
                Vector3 p2_candidate = { x, 0.0f, min(maxZ, halfGridSize) };

                // 線分全体が完全に視錐台の外にある場合はスキップ
                if (IsPointInFrustum(p1_candidate) || IsPointInFrustum(p2_candidate) || IsAABBInFrustum({ p1_candidate.x, -0.1f, p1_candidate.z }, { p2_candidate.x, 0.1f, p2_candidate.z })) {
                    gridVertices_.push_back({ {p1_candidate.x, p1_candidate.y, p1_candidate.z, 1.0f} });
                    gridVertices_.push_back({ {p2_candidate.x, p2_candidate.y, p2_candidate.z, 1.0f} });
                }
            }
        }

        // 頂点データが更新されたら、頂点バッファも更新
        if (!gridVertices_.empty()) {
            Microsoft::WRL::ComPtr<ID3D12Device> device = DirectXCommon::GetInstance()->GetDevice();
            // 既存のバッファを解放し、新しいバッファを作成
            gridVertexBuffer_.Reset(); // Resetで以前のリソースを解放
            gridVertexBuffer_ = CreateBufferResource(device.Get(), sizeof(GridVertex) * gridVertices_.size());

            GridVertex* mappedGridVertices = nullptr;
            HRESULT hr = gridVertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedGridVertices));
            assert(SUCCEEDED(hr));
            std::copy(gridVertices_.begin(), gridVertices_.end(), mappedGridVertices);
            gridVertexBuffer_->Unmap(0, nullptr);

            gridVbView_.BufferLocation = gridVertexBuffer_->GetGPUVirtualAddress();
            gridVbView_.SizeInBytes = static_cast<UINT>(sizeof(GridVertex) * gridVertices_.size());
            gridVbView_.StrideInBytes = sizeof(GridVertex);
        } else {
            // 描画する頂点がない場合、ビューをクリアしておく
            gridVbView_ = {};
        }
    }

    void Grid::GenerateCenteredGridVertices(const Vector3& centerPosition) {
        gridVertices_.clear();
        const float step = gridSize_ / static_cast<float>(numDivisions_);
        const float halfGridSize = gridSize_ / 2.0f;

        // グリッドの相対的な開始・終了座標を計算
        // これらは常に [-halfGridSize, halfGridSize] の範囲になります
        float relativeMin = -halfGridSize;
        float relativeMax = halfGridSize;

        // X軸に平行な線
        for (int i = 0; i <= numDivisions_; ++i) {
            float relativeZ = relativeMin + i * step;
            Vector3 p1_relative = { relativeMin, 0.0f, relativeZ };
            Vector3 p2_relative = { relativeMax, 0.0f, relativeZ };

            // ワールド座標に変換
            Vector3 p1_world = Add(p1_relative, centerPosition);
            Vector3 p2_world = Add(p2_relative, centerPosition);

            // この線分をカバーするAABBを作成 (グリッドはY=0平面なので、わずかにY方向の厚みを持たせる)
            Vector3 aabbMin = { min(p1_world.x, p2_world.x), -0.1f, min(p1_world.z, p2_world.z) };
            Vector3 aabbMax = { max(p1_world.x, p2_world.x), 0.1f, max(p1_world.z, p2_world.z) };

            // AABBが視錐台と交差する場合のみ頂点を追加
            if (IsAABBInFrustum(aabbMin, aabbMax)) {
                gridVertices_.push_back({ {p1_world.x, p1_world.y, p1_world.z, 1.0f} });
                gridVertices_.push_back({ {p2_world.x, p2_world.y, p2_world.z, 1.0f} });
            }
        }
        // Z軸に平行な線
        for (int i = 0; i <= numDivisions_; ++i) {
            float relativeX = relativeMin + i * step;
            Vector3 p1_relative = { relativeX, 0.0f, relativeMin };
            Vector3 p2_relative = { relativeX, 0.0f, relativeMax };

            // ワールド座標に変換
            Vector3 p1_world = Add(p1_relative, centerPosition);
            Vector3 p2_world = Add(p2_relative, centerPosition);

            Vector3 aabbMin = { min(p1_world.x, p2_world.x), -0.1f, min(p1_world.z, p2_world.z) };
            Vector3 aabbMax = { max(p1_world.x, p2_world.x), 0.1f, max(p1_world.z, p2_world.z) };

            if (IsAABBInFrustum(aabbMin, aabbMax)) {
                gridVertices_.push_back({ {p1_world.x, p1_world.y, p1_world.z, 1.0f} });
                gridVertices_.push_back({ {p2_world.x, p2_world.y, p2_world.z, 1.0f} });
            }
        }

        // 頂点データが更新されたら、頂点バッファも更新
        if (!gridVertices_.empty()) {
            Microsoft::WRL::ComPtr<ID3D12Device> device = DirectXCommon::GetInstance()->GetDevice();
            gridVertexBuffer_.Reset(); // 以前のリソースを解放
            gridVertexBuffer_ = CreateBufferResource(device.Get(), sizeof(GridVertex) * gridVertices_.size());

            GridVertex* mappedGridVertices = nullptr;
            HRESULT hr = gridVertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedGridVertices));
            assert(SUCCEEDED(hr));
            std::copy(gridVertices_.begin(), gridVertices_.end(), mappedGridVertices);
            gridVertexBuffer_->Unmap(0, nullptr);

            gridVbView_.BufferLocation = gridVertexBuffer_->GetGPUVirtualAddress();
            gridVbView_.SizeInBytes = static_cast<UINT>(sizeof(GridVertex) * gridVertices_.size());
            gridVbView_.StrideInBytes = sizeof(GridVertex);
        } else {
            gridVbView_ = {};
        }
    }

    bool Grid::IsAABBInFrustum(const Vector3& minBounds, const Vector3& maxBounds) {
        // AABBの8つの頂点を定義
        Vector3 corners[8] = {
            {minBounds.x, minBounds.y, minBounds.z},
            {maxBounds.x, minBounds.y, minBounds.z},
            {minBounds.x, maxBounds.y, minBounds.z},
            {maxBounds.x, maxBounds.y, minBounds.z},
            {minBounds.x, minBounds.y, maxBounds.z},
            {maxBounds.x, minBounds.y, maxBounds.z},
            {minBounds.x, maxBounds.y, maxBounds.z},
            {maxBounds.x, maxBounds.y, maxBounds.z}
        };

        // 各平面に対して、AABBの全ての頂点が平面の外側にあるかをチェック
        for (int i = 0; i < 6; ++i) { // 6つの視錐台平面
            bool allOut = true;
            for (int j = 0; j < 8; ++j) { // AABBの8つの頂点
                // (法線ベクトル・点) + 距離 >= 0 の場合、平面の内側
                if (Dot(currentFrustum_.planes[i].normal, corners[j]) + currentFrustum_.planes[i].distance >= 0) {
                    allOut = false;
                    break;
                }
            }
            if (allOut) {
                return false; // AABBの全ての頂点がこの平面の外側にあるため、完全に視錐台の外
            }
        }
        return true; // 少なくとも一部が視錐台内にある、または交差している
    }

    bool Grid::IsPointInFrustum(const Vector3& point) {
        for (int i = 0; i < 6; ++i) {
            // 点が平面の外側にあるかチェック
            // (法線ベクトル・点) + 距離 < 0 の場合、平面の外側
            if (Dot(currentFrustum_.planes[i].normal, point) + currentFrustum_.planes[i].distance < 0) {
                return false; // 1つの平面の外側にあれば、視錐台の外
            }
        }
        return true; // 全ての平面の内側にある
    }

} // namespace BonjinEngine