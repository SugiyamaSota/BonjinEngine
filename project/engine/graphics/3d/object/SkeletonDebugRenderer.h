#pragma once

#include <cstddef>
#include <d3d12.h>
#include <wrl/client.h>

#include "Skeleton.h"

class Camera;
class DirectXCommon;

class SkeletonDebugRenderer {
public:
	SkeletonDebugRenderer() = default;
	~SkeletonDebugRenderer();

	void Initialize(size_t maxJointCount);
	void Update(
		const Skeleton& skeleton,
		const Matrix4x4& worldMatrix,
		const Camera* camera);
	void Draw();

	void SetColor(const Vector4& color) { color_ = color; }

private:
	struct LineVertex {
		Vector4 position;
		Vector4 color;
	};

	DirectXCommon* common_ = nullptr;
	ID3D12Device* device_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> matrixResource_;
	LineVertex* vertexData_ = nullptr;
	Matrix4x4* matrixData_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	size_t maxVertexCount_ = 0;
	uint32_t vertexCount_ = 0;
	Vector4 color_{ 0.1f, 1.0f, 0.2f, 1.0f };
};
