#include "ModelBuilder.h"

#include <fstream>

ModelData ModelBuilder::LoadModelFile(const std::string& directoryPath, const std::string& filename) {
	Assimp::Importer importer;
	std::string fullPath = directoryPath + '/' + filename;
	const aiScene* scene = importer.ReadFile(fullPath, aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes());

	ModelData modelData;

	modelData.rootNode = ReadNode(scene->mRootNode);

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		const aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasTextureCoords(0));
		assert(mesh->HasNormals());

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			const aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形であることを確認

			for (uint32_t element = 0; element < face.mNumIndices; ++element) {

				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D position = mesh->mVertices[vertexIndex];
				aiVector3D normal = mesh->mNormals[vertexIndex];
				aiVector3D texcoord = mesh->mTextureCoords[0][vertexIndex];
				VertexData vertex;
				vertex.position = { position.x, position.y, position.z, 1.0f };
				vertex.normal = { normal.x, normal.y, normal.z };
				vertex.texcoord = { texcoord.x, texcoord.y };

				vertex.position.x *= -1.f;
				vertex.normal.x *= -1.f;

				modelData.vertices.push_back(vertex);

			}
		}

		for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
			aiMaterial* material = scene->mMaterials[materialIndex];

			if(material->GetTextureCount(aiTextureType_DIFFUSE)!=0) {
				aiString texturePath;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
				modelData.material.textureFilepath = directoryPath + '/' + std::string(texturePath.C_Str());
			}
		}
	}

	return modelData;
}

ModelData ModelBuilder::CreateSphereModel(uint32_t subdivision) {
	ModelData modelData;
	const float kPi = 3.1415926535f;

	// 1. 頂点データの生成
	// 緯度方向に分割 (kSubdivision)
	for (uint32_t lat = 0; lat <= subdivision; ++lat) {
		float phi = -kPi / 2.0f + kPi * float(lat) / float(subdivision);

		// 経度方向に分割 (kSubdivision)
		for (uint32_t lon = 0; lon <= subdivision; ++lon) {
			float theta = 2.0f * kPi * float(lon) / float(subdivision);

			VertexData vertex;
			// 座標 (半径1.0)
			vertex.position.x = std::cos(phi) * std::cos(theta);
			vertex.position.y = std::sin(phi);
			vertex.position.z = std::cos(phi) * std::sin(theta);
			vertex.position.w = 1.0f;

			// 法線 (中心からの方向)
			vertex.normal = { vertex.position.x, vertex.position.y, vertex.position.z };

			// テクスチャ座標
			vertex.texcoord.x = float(lon) / float(subdivision);
			vertex.texcoord.y = 1.0f - float(lat) / float(subdivision);

			// 一旦リストに保持（インデックス計算のため）
			// ※ModelDataの構造に合わせて、後で三角形としてpush_backします
		}
	}

	// 2. インデックスを元に三角形を作成
	// ※LoadObjFileの仕様に合わせ、頂点配列を直接構成します
	for (uint32_t lat = 0; lat < subdivision; ++lat) {
		for (uint32_t lon = 0; lon < subdivision; ++lon) {
			uint32_t start = lat * (subdivision + 1) + lon;

			// 各格子における4つの頂点インデックス
			// p0 --- p1
			// |      |
			// p2 --- p3
			uint32_t p0 = start;
			uint32_t p1 = start + 1;
			uint32_t p2 = start + (subdivision + 1);
			uint32_t p3 = start + (subdivision + 1) + 1;

			// 三角形1 (p0, p1, p2)
			modelData.vertices.push_back(GetSphereVertex(p0, subdivision));
			modelData.vertices.push_back(GetSphereVertex(p1, subdivision));
			modelData.vertices.push_back(GetSphereVertex(p2, subdivision));

			// 三角形2 (p1, p3, p2)
			modelData.vertices.push_back(GetSphereVertex(p1, subdivision));
			modelData.vertices.push_back(GetSphereVertex(p3, subdivision));
			modelData.vertices.push_back(GetSphereVertex(p2, subdivision));
		}
	}

	

	return modelData;
}

MaterialData ModelBuilder::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + '/' + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		//identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			//連結してファイルパスにする
			materialData.textureFilepath = directoryPath + '/' + textureFilename;
		}
	}
	return materialData;
}

VertexData ModelBuilder::GetSphereVertex(uint32_t index, uint32_t subdivision) {
	const float kPi = 3.1415926535f;
	uint32_t lat = index / (subdivision + 1);
	uint32_t lon = index % (subdivision + 1);

	float phi = -kPi / 2.0f + kPi * float(lat) / float(subdivision);
	float theta = 2.0f * kPi * float(lon) / float(subdivision);

	VertexData v;
	v.position = { std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta), 1.0f };
	v.normal = { v.position.x, v.position.y, v.position.z };
	v.texcoord = { float(lon) / float(subdivision), 1.0f - float(lat) / float(subdivision) };
	return v;
}

Node ModelBuilder::ReadNode(aiNode* node) {
	Node result;

	aiMatrix4x4 aiLocalMatrix = node->mTransformation;
	aiLocalMatrix.Transpose(); // Assimpは行優先、DirectXは列優先なので転置する
	result.localMatrix.m[0][0] = aiLocalMatrix[0][0];
	result.localMatrix.m[0][1] = aiLocalMatrix[0][1];
	result.localMatrix.m[0][2] = aiLocalMatrix[0][2];
	result.localMatrix.m[0][3] = aiLocalMatrix[0][3];
	result.localMatrix.m[1][0] = aiLocalMatrix[1][0];
	result.localMatrix.m[1][1] = aiLocalMatrix[1][1];
	result.localMatrix.m[1][2] = aiLocalMatrix[1][2];
	result.localMatrix.m[1][3] = aiLocalMatrix[1][3];
	result.localMatrix.m[2][0] = aiLocalMatrix[2][0];
	result.localMatrix.m[2][1] = aiLocalMatrix[2][1];
	result.localMatrix.m[2][2] = aiLocalMatrix[2][2];
	result.localMatrix.m[2][3] = aiLocalMatrix[2][3];
	result.localMatrix.m[3][0] = aiLocalMatrix[3][0];
	result.localMatrix.m[3][1] = aiLocalMatrix[3][1];
	result.localMatrix.m[3][2] = aiLocalMatrix[3][2];
	result.localMatrix.m[3][3] = aiLocalMatrix[3][3];

	result.name = node->mName.C_Str();
	result.children.reserve(node->mNumChildren);
	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		result.children.push_back(ReadNode(node->mChildren[i]));
	}

	return result;

}

