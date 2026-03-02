#pragma once
#include <cstdint>
#include <string>

#include "Calc.h"

// テクスチャ
struct MaterialData {
	std::string textureFilePath;
	uint32_t textureIndex = 0;
	Vector3 emissive;
};
// 頂点データ
struct VertexData {
	Vector4 position; // 頂点座標
	Vector2 texcoord; // テクスチャ座標
	Vector3 normal; // 正規化座標
	Vector3 outlineNormal;   // 第二法線
};
// モデルデータ
struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};
// マテリアルデータ
struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
	Vector3 emissive;
	float padding2; // バイト合わせ
};
