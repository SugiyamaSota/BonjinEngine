#include "ModelBuilder.h"

#include <fstream>

ModelData ModelBuilder::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4>positions;
	std::vector<Vector3>normals;
	std::vector<Vector2>texcoords;
	std::string line;

	std::ifstream file(directoryPath + '/' + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1.0f;
			position.w = 1.0f;
			positions.push_back(position);
		} else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f;
			normals.push_back(normal);
		} else if (identifier == "f") {
			VertexData triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				triangle[faceVertex] = { position,texcoord,normal };
			}
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;

			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
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

