#pragma once
#include <cstdint>
#include <string>

#include "Calc.h"
#include <map>
#include <span>
#include <optional>
#include <array>

//キーフレーム
template<typename tValue>
struct Keyframe
{
	float time;//キーフレームの時刻
	tValue value;//キーフレームの幅
};
using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

template<typename tValue>

struct AnimationCurve
{
	std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation
{
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion>rotate;
	AnimationCurve<Vector3> scale;
};

struct Animation
{
	float duration;//アニメーション全体の尺(単位は秒)
	//NodeAnimationの集合。Node名でひけるようにしておく
	std::map<std::string, NodeAnimation>nodeAnimations;
};

struct Joint
{
	QuaternionTransform transform;//Transform情報
	Matrix4x4 localMatrix;
	Matrix4x4 skeletonSpaceMatrix;//skeletonSpaceでの変換行列
	std::string name;
	std::vector<int32_t> children;//子JointのIndexのリスト。いなければ空
	int32_t index;
	std::optional<int32_t>parent;//親JointのIndex、いなければnull
};

struct Skeleton
{
	int32_t root;//RootJointのIndex
	std::map<std::string, int32_t>jointMap;//Joint名とIndexとの辞書
	std::vector<Joint>joints;//所属しているジョイント
};

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

struct Node
{
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};

struct VertexWeightData
{
	float weight;
	uint32_t vertexIndex;
};

struct JointWeightData
{
	Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData> vertexWeights;
};
static const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence
{
	std::array<float, kNumMaxInfluence> weights;
	std::array<int32_t, kNumMaxInfluence>jointIndices;
};

struct WellForGPU
{
	Matrix4x4 skeletonSpaceMatrix;//位置用
	Matrix4x4 skeletonSpaceInverseTransposeMatrix;//法線用
};

struct SkinCluster
{
	std::vector<Matrix4x4>inverseBindPoseMatrices;
	Microsoft::WRL::ComPtr<ID3D12Resource>influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
	std::span<VertexInfluence>mappedInfluence;
	Microsoft::WRL::ComPtr<ID3D12Resource>paletteResource;
	std::span<WellForGPU>mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>paletteSrvHandle;

};

// モデルデータ
struct ModelData {
    std::map<std::string, JointWeightData> skinClusterData;
	std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
	MaterialData material;
    Node rootNode;
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
