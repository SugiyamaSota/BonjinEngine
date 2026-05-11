#pragma once
#include"BonjinEngine.h"

#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

class ModelBuilder
{
public:
	enum class ModelType {
		kSphere,
		kCube,
		kPlane
	};

	/// <summary>
	/// objファイルの読み込み
	/// </summary>
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

	static ModelData CreateModel(ModelType type);

	static ModelData CreateSphereModel(uint32_t subdivision);

	static ModelData CreateCubeModel();

	static ModelData CreatePlaneModel();

private:
	/// <summary>
	/// mtlファイルの読み込み
	/// </summary>
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	static VertexData GetSphereVertex(uint32_t index, uint32_t subdivision);

	static Node ReadNode(aiNode* node);
};

