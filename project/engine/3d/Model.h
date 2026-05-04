#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <D3d12.h>
#include <cassert>
#include <wrl.h>
#include <dxcapi.h>
#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

#include "Calc.h"
#include "CommonStructs.h"

class ModelCommon;
class DirectXCommon;

class Model {
public:
	// 初期化
	void Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon,const std::string& directoryPath,const std::string& filename);
	// 描画
	void Draw();


	// .mtlファイルの読み込み
	//MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	// .objファイルの読み込み
	ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

	// 第二法線生成
	void GenerateOutlineNormal(std::vector<VertexData>& vertices);

	//アニメーション スケルトン生成用関数
	SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
		const Skeleton& skeleton, const ModelData& modelData, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize);

	Skeleton CreateSkeleton(const Node& rootNode);

	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

	//アニメーションの解析
	static Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

	static Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);
	static Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);

	Node ReadNode(aiNode* node);

	//skeletonの更新
	void Update(Skeleton& skeleton);
	void Update(SkinCluster& skinCluster, const Skeleton& skeleton);

	//アニメーション適用
	void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime);

	// スキニングモデル描画用（骨あり）
	void Draw(const SkinCluster& skinCluster);

	ModelData& GetModelData() { return modelData; }
private:
	// Objファイルのデータ
	ModelData modelData;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	Material* materialData = nullptr;

	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;


	// ModelCommonのポインタ
	ModelCommon* modelCommon_ = nullptr;
	// DirectXCommonのポインタ
	DirectXCommon* dxCommon_ = nullptr;

};

