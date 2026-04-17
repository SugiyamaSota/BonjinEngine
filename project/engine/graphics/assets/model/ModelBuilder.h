#pragma once
#include"BonjinEngine.h"

#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

class ModelBuilder
{
public:
	/// <summary>
	/// objファイルの読み込み
	/// </summary>
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// 球体のモデルを生成
	/// </summary>
	/// <param name="subdivision"></param>
	/// <returns></returns>
	static ModelData CreateSphereModel(uint32_t subdivision);

private:
	/// <summary>
	/// mtlファイルの読み込み
	/// </summary>
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	static VertexData GetSphereVertex(uint32_t index, uint32_t subdivision);
};

