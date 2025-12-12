#pragma once

struct ID3D12Device;
struct ID3D12PipelineState;
class DirectXCommon;

namespace Bonjin {

	class SpriteCommon
	{
	public:

		// コンストデスト
		SpriteCommon();
		~SpriteCommon();

		// 初期化
		void Initialize();

		// 更新処理(?)
		void  Update();

		// PSOなどの描画前設定
		void PreDraw();

		// 登録した全スプライトを描画
		void PostDraw();

	private:

		// dxCommon変数
		DirectXCommon* dxCommon_;

		// デバイス
		ID3D12Device* device_;

		// スプライト用pso
		ID3D12PipelineState* pso_;

	};

}
