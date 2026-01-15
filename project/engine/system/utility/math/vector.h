#pragma once

///// 前方宣言 /////
struct Vector2;
struct Vector3;
struct Vector4;

///// 計算関数 /////
//加算
Vector3 Add(const Vector3& vector1, const Vector3& vector2);

//減算
Vector3 Subtract(const Vector3& vector1, const Vector3& vector2);

//スカラー倍
Vector3 Multiply(float scalar, const Vector3& vector);

//内積
float Dot(const Vector3& vector1, const Vector3& vector2);

//長さ(ノルム)
float Length(const Vector3& vector);

//正規化
Vector3 Normalize(const Vector3& vector);

//クロス積
Vector3 Cross(const Vector3& vector1, const Vector3& vector2);

Vector3 Lerp(const Vector3& start, const Vector3& end, float t);
