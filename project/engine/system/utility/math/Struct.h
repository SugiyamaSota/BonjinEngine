#pragma once
#include<stdint.h>
#include<string>
#include<vector>

struct Vector2 {
	float x, y;
};

struct Vector3 {
	float x, y, z;
};

struct Vector4 {
	float x, y, z, w;
};

struct Quaternion {
	float x, y, z, w;
};

struct Matrix4x4 {
	float m[4][4];
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material {
	Vector4 color;                   // 16バイト (0～15) [ぴったり]

	int32_t enableLighting;          // 4バイト  (16～19)
	int32_t enableSpecular;          // 4バイト  (20～23)
	float padding1[2];               // 8バイト  (24～31) ➔ 次の行列を16アライメントへ

	Matrix4x4 uvTransform;           // 64バイト (32～95) [ぴったり]

	float shininess;                 // 4バイト  (96～99)
	int32_t enableEnvironmentMap;    // 4バイト  (100～103)
	float environmentCoefficient;    // 4バイト  (104～107)
	float padding2;                  // 4バイト  (108～111) ➔ ここまでの4つで合計16バイト
}; // 構造体全体のサイズ：112バイト (16の倍数)

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Matrix4x4 WorldInverseTranspose;
};

struct MaterialData {
	std::string textureFilepath;
};

struct Node {
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
	Node rootNode;
};

struct WorldTransform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
	Matrix4x4 worldMat;
	WorldTransform* parent;
};

//平面
struct Plane {
	Vector3 normal;
	float distance;
};

// 光
struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

// ポイント
struct PointLight {
	Vector4 color;
	Vector3 position;
	float intensity;
	float radius; // ライトが届く最大距離
	float decay;  // 減衰率
	float padding[2];
};

// スポット
struct SpotLight{
	Vector4 color; // 色
	Vector3 position; // 位置
	float intensity; // 輝度
	Vector3 direction; // 方向
	float distance; // ライトが届く最大距離
	float decay; // 減衰率
	float cosAngle; // スポットライトの余弦
	float padding[2];
};



struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

struct AABB {
	Vector3 min;
	Vector3 max;
};

struct CameraForGPU {
	Vector3 worldPosition;
};
