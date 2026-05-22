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

ModelData ModelBuilder::CreateModel(ModelType type) {
	switch (type) {
	case ModelType::kSphere:
		return CreateSphereModel(16); // 分割数はデフォルト値を指定
	case ModelType::kCube:
		return CreateCubeModel();
	case ModelType::kSkyBox:
		return CreateSkyBoxModel();
	case ModelType::kPlane:
		return CreatePlaneModel();
	case ModelType::kRing: // 追加
		return CreateRingModel();
	case ModelType::kCylinder:
		return CreateCylinderModel();
	default:
		assert(false && "未定義のモデルタイプです");
		return ModelData();
	}
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

ModelData ModelBuilder::CreateCubeModel() {
	ModelData modelData;

	auto addFaceDirect = [&](const std::vector<VertexData>& vertices, const std::vector<int>& indices) {
		for (int index : indices) {
			// インデックスに基いて、位置・UV・法線が含まれた頂点データをそのまま追加する
			modelData.vertices.push_back(vertices[index]);
		}
		};

	addFaceDirect(
		{
			{ { 1.0f,  1.0f,  1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} }, // 左上
			{ { 1.0f,  1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} }, // 右上
			{ { 1.0f, -1.0f,  1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} }, // 左下
			{ { 1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} }  // 右下
		},
		{ 0, 2, 1,  1, 2, 3 }
	);

	// 左面 (x = -1.0f) -> 法線は Xマイナス方向 (-1, 0, 0)
	addFaceDirect(
		{
			{ {-1.0f,  1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} }, // 左上
			{ {-1.0f,  1.0f,  1.0f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} }, // 右上
			{ {-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} }, // 左下
			{ {-1.0f, -1.0f,  1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} }  // 右下
		},
		{ 0, 2, 1,  1, 2, 3 }
	);

	// 前面 (z = 1.0f) -> 法線は Zプラス方向 (0, 0, 1)
	addFaceDirect(
		{
			{ {-1.0f,  1.0f,  1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // 左上
			{ { 1.0f,  1.0f,  1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // 右上
			{ {-1.0f, -1.0f,  1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // 左下
			{ { 1.0f, -1.0f,  1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }  // 右下
		},
		{ 0, 2, 1,  1, 2, 3 }
	);

	// 背面 (z = -1.0f) -> 法線は Zマイナス方向 (0, 0, -1)
	addFaceDirect(
		{
			{ { 1.0f,  1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 左上
			{ {-1.0f,  1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 右上
			{ { 1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }, // 左下
			{ {-1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }  // 右下
		},
		{ 0, 2, 1,  1, 2, 3 }
	);

	// 上面 (y = 1.0f) -> 法線は Yプラス方向 (0, 1, 0)
	addFaceDirect(
		{
			{ {-1.0f,  1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} }, // 左上
			{ { 1.0f,  1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} }, // 右上
			{ {-1.0f,  1.0f,  1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} }, // 左下
			{ { 1.0f,  1.0f,  1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f} }  // 右下
		},
		{ 0, 2, 1,  1, 2, 3 }
	);

	// 下面 (y = -1.0f) -> 法線は Yマイナス方向 (0, -1, 0)
	addFaceDirect(
		{
			{ {-1.0f, -1.0f,  1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} }, // 左上
			{ { 1.0f, -1.0f,  1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f} }, // 右上
			{ {-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} }, // 左下
			{ { 1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f} }  // 右下
		},
		{ 0, 2, 1,  1, 2, 3 }
	);

	return modelData;
}

ModelData ModelBuilder::CreateSkyBoxModel() {
	ModelData modelData; auto addFaceDirect = [&](std::vector<Vector4> p, std::vector<int> indices) {

		for (int index : indices) {

			VertexData v;

			v.position = p[index];

			// Skyboxではnormalもtexcoordも使わないので適当に埋める

			v.normal = { 0, 0, 0 };

			v.texcoord = { 0, 0 };

			modelData.vertices.push_back(v);

		}

		};// --- 各面のデータ定義 (資料の座標と順序に準拠) ---// 右面 (x = 1.0f)

	addFaceDirect(

		{ {1,1,1,1}, {1,1,-1,1}, {1,-1,1,1}, {1,-1,-1,1} },

		{ 0,1,2, 2,1,3 } // 内側を向く順序

	);// 左面 (x = -1.0f)

	addFaceDirect(

		{ {-1,1,-1,1}, {-1,1,1,1}, {-1,-1,-1,1}, {-1,-1,1,1} },

		{ 0,1,2, 2,1,3 }

	);// 前面 (z = 1.0f)

	addFaceDirect(

		{ {-1,1,1,1}, {1,1,1,1}, {-1,-1,1,1}, {1,-1,1,1} },

		{ 0,1,2, 2,1,3 }

	);// 背面 (z = -1.0f)

	addFaceDirect(

		{ {1,1,-1,1}, {-1,1,-1,1}, {1,-1,-1,1}, {-1,-1,-1,1} },

		{ 0,1,2, 2,1,3 }

	);// 上面 (y = 1.0f)

	addFaceDirect(

		{ {-1,1,-1,1}, {1,1,-1,1}, {-1,1,1,1}, {1,1,1,1} },

		{ 0,1,2, 2,1,3 }

	);// 下面 (y = -1.0f)

	addFaceDirect(

		{ {-1,-1,1,1}, {1,-1,1,1}, {-1,-1,-1,1}, {1,-1,-1,1} },

		{ 0,1,2, 2,1,3 }

	); return modelData;
}

ModelData ModelBuilder::CreatePlaneModel() {
	ModelData modelData;

	// 左下、左上、右下、右上の4頂点
	// Zは0で固定（XY平面）
	std::vector<VertexData> vertices = {
		{ {-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }, // 左下
		{ {-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 左上
		{ { 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }, // 右下
		{ { 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 右上
	};

	// 三角形1 (左下、左上、右下)
	modelData.vertices.push_back(vertices[0]);
	modelData.vertices.push_back(vertices[1]);
	modelData.vertices.push_back(vertices[2]);

	// 三角形2 (右下、左上、右上)
	modelData.vertices.push_back(vertices[2]);
	modelData.vertices.push_back(vertices[1]);
	modelData.vertices.push_back(vertices[3]);

	return modelData;
}

ModelData ModelBuilder::CreateRingModel(uint32_t subdivision, float innerRadius, float outerRadius) {
	ModelData modelData;
	const float kPi = 3.1415926535f;
	// スライド：2.0f * pi / 分割数
	float radianPerDivide = 2.0f * kPi / float(subdivision);

	for (uint32_t index = 0; index < subdivision; ++index) {
		// スライド通りの計算式
		float sin = std::sin(float(index) * radianPerDivide);
		float cos = std::cos(float(index) * radianPerDivide);
		float sinNext = std::sin(float(index + 1) * radianPerDivide);
		float cosNext = std::cos(float(index + 1) * radianPerDivide);

		// スライドの u (回転方向) を計算
		float u = float(index) / float(subdivision);
		float uNext = float(index + 1) / float(subdivision);

		// --- 頂点データの構築 (スライドの①〜④に準拠) ---
		// ※テクスチャをV方向にするため、第2引数に 0.0f(外側) と 1.0f(内側) を設定

		// ① 外側・現在
		VertexData v1;
		v1.position = { -sin * outerRadius, cos * outerRadius, 0.0f, 1.0f };
		v1.texcoord = { u, 0.0f };
		v1.normal = { 0.0f, 0.0f, -1.0f };

		// ② 外側・次
		VertexData v2;
		v2.position = { -sinNext * outerRadius, cosNext * outerRadius, 0.0f, 1.0f };
		v2.texcoord = { uNext, 0.0f };
		v2.normal = { 0.0f, 0.0f, -1.0f };

		// ③ 内側・現在
		VertexData v3;
		v3.position = { -sin * innerRadius, cos * innerRadius, 0.0f, 1.0f };
		v3.texcoord = { u, 1.0f }; // 内側をV=1.0にする
		v3.normal = { 0.0f, 0.0f, -1.0f };

		// ④ 内側・次
		VertexData v4;
		v4.position = { -sinNext * innerRadius, cosNext * innerRadius, 0.0f, 1.0f };
		v4.texcoord = { uNext, 1.0f };
		v4.normal = { 0.0f, 0.0f, -1.0f };

		// --- 三角形の構築 (時計回り) ---

		// 三角形1: ① -> ② -> ③
		modelData.vertices.push_back(v1);
		modelData.vertices.push_back(v2);
		modelData.vertices.push_back(v3);

		// 三角形2: ③ -> ② -> ④
		modelData.vertices.push_back(v3);
		modelData.vertices.push_back(v2);
		modelData.vertices.push_back(v4);
	}

	return modelData;
}

ModelData ModelBuilder::CreateCylinderModel() {
	ModelData modelData;

	// スライドの定数定義
	const uint32_t kCylinderDivide = 32;
	const float kTopRadius = 1.0f;
	const float kBottomRadius = 1.0f;
	const float kHeight = 2.0f;
	const float kPi = 3.1415926535f;
	const float radianPerDivide = 2.0f * kPi / float(kCylinderDivide);

	for (uint32_t index = 0; index < kCylinderDivide; ++index) {
		float sin = std::sin(float(index) * radianPerDivide);
		float cos = std::cos(float(index) * radianPerDivide);
		float sinNext = std::sin(float(index + 1) * radianPerDivide);
		float cosNext = std::cos(float(index + 1) * radianPerDivide);

		float u = float(index) / float(kCylinderDivide);
		float uNext = float(index + 1) / float(kCylinderDivide);

		// --- スライドのデータを元に6つの頂点を生成して push_back ---

		// 1つ目の三角形
		VertexData v1, v2, v3;
		// kHeight と 0.0f を入れ替える
		v1.position = { -sin * kTopRadius, 0.0f, cos * kTopRadius, 1.0f }; // kHeight から 0.0f に
		v1.texcoord = { u, 0.0f };
		v1.normal = { -sin, 0.0f, cos };

		v2.position = { -sinNext * kTopRadius, 0.0f, cosNext * kTopRadius, 1.0f }; // kHeight から 0.0f に
		v2.texcoord = { uNext, 0.0f };
		v2.normal = { -sinNext, 0.0f, cosNext };

		v3.position = { -sin * kBottomRadius, kHeight, cos * kBottomRadius, 1.0f }; // 0.0f から kHeight に
		v3.texcoord = { u, 1.0f };
		v3.normal = { -sin, 0.0f, cos };

		// ★重要：上下をひっくり返すと「時計回り」の順序が変わるため、
		// 裏返らないように v1 -> v3 -> v2 の順番で push します
		modelData.vertices.push_back(v1);
		modelData.vertices.push_back(v3);
		modelData.vertices.push_back(v2);


		// 2つ目の三角形
		VertexData v4, v5, v6;
		v4.position = { -sin * kBottomRadius, kHeight, cos * kBottomRadius, 1.0f }; // 0.0f から kHeight に
		v4.texcoord = { u, 1.0f };
		v4.normal = { -sin, 0.0f, cos };

		v5.position = { -sinNext * kTopRadius, 0.0f, cosNext * kTopRadius, 1.0f }; // kHeight から 0.0f に
		v5.texcoord = { uNext, 0.0f };
		v5.normal = { -sinNext, 0.0f, cosNext };

		v6.position = { -sinNext * kBottomRadius, kHeight, cosNext * kBottomRadius, 1.0f }; // 0.0f から kHeight に
		v6.texcoord = { uNext, 1.0f };
		v6.normal = { -sinNext, 0.0f, cosNext };

		// ★こちらも順序を v4 -> v6 -> v5 に入れ替えて push します
		modelData.vertices.push_back(v4);
		modelData.vertices.push_back(v6);
		modelData.vertices.push_back(v5);
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

