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

struct Matrix4x4 {
	float m[4][4];
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material {
	Vector4 color;          // 16バイト
	int32_t enableLighting; // 4バイト
	float padding[3];       // 12バイト (uvTransformを16バイト境界に合わせる)
	Matrix4x4 uvTransform;  // 64バイト
	float shininess;        // 4バイト
	float padding2[3];      // 12バイト (構造体サイズを16の倍数にする)
}; 

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct MaterialData {
	std::string textureFilepath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
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
	float intentity;
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
