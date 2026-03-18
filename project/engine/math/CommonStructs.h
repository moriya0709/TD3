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
    int enableLighting;
    int enableToonShading;
    Vector2 pad1;
    Matrix4x4 uvTransform;
    Vector3 emissive;
    float shininess;

    // フレネル反射 / リムライト関連
    Vector4 fresnelColor; // トゥーンOFF時のフレネル反射の色
    float fresnelPower; // フレネル反射の累乗数（値が大きいほどエッジに寄る）
    Vector4 rimColor; // トゥーンON時のリムライトの色
    float rimThreshold; // リムライトの境界（0～1、値が大きいほど細くなる）
    float pad3[3]; // バイト合わせ
};
