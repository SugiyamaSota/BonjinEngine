#include "SpriteCommon.h"

#include"../system/utility/common/DirectXCommon.h"

using namespace Bonjin;

SpriteCommon::SpriteCommon()
{

}

SpriteCommon::~SpriteCommon()
{

}

void SpriteCommon::Initialize()
{

	dxCommon_ = DirectXCommon::GetInstance();
	device_ = dxCommon_->GetDevice();

	pso_ =
		dxCommon_->GetPSO()->GetPipelineState(
			device_,
			PrimitiveType::kModel,
			BlendMode::kNormal,
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK
		);

}

void SpriteCommon::PreDraw()
{

	// ルートシグネチャ
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetPSO()->GetRootSignature(PrimitiveType::kModel));

	// psセット
	dxCommon_->GetCommandList()->SetPipelineState(pso_);

	// プリミティブトポロジの設定 (三角形リスト)
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

void SpriteCommon::PostDraw() {

}
