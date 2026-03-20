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
    Vector4 color;             // 16バイト
    int32_t enableLighting;    // 4バイト (★boolではなく必ずint32_tにする)
    int32_t enableToonShading; // 4バイト (★boolではなく必ずint32_tにする)
    Vector2 pad1;              // 8バイト (隙間埋め)
    Matrix4x4 uvTransform;     // 64バイト
    Vector3 emissive;          // 12バイト (★ここに10.29が入る)
    float shininess;           // 4バイト

    Vector4 fresnelColor;      // 16バイト
    float fresnelPower;        // 4バイト
    float pad2[3];             // 12バイト (★超重要：次のrimColorを16の倍数に押し出すための隙間埋め)

    Vector4 rimColor;          // 16バイト
    float rimThreshold;        // 4バイト
    float pad3[3];             // 12バイト (隙間埋め)
};
