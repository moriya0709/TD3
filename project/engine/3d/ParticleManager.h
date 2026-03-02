#pragma once
#include <string>
#include <list>
#include <D3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <numbers>
#include <vector>
#include <fstream>
#include <dxcapi.h>
#include <random>

#include "Calc.h"
#include "CommonStructs.h"

class DirectXCommon;
class SrvManager;
class Camera;

// 乱数生成器
extern std::random_device seedGenerator;
extern std::mt19937 randomEngine;

// パーティクル
struct Particle {
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
	Matrix4x4 wvp;
	Matrix4x4 world;
};
// 場(加速度)
struct AccelerationField {
	Vector3 acceleration; //!< 加速度
	AABB area; //!< 範囲
};
// パーティクル描画用データ(GPU用)
struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 world;
	Vector4 color;
};

class ParticleManager {
public:
	struct ParticleGroup {
		MaterialData material;
		std::list<Particle> particles;
		uint32_t  instancingIndex;
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
		uint32_t instancingCount;
		ParticleForGPU* instancingData;
	};
	std::unordered_map<std::string, ParticleGroup> particleGroups;
	

	// 初期化
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, const std::string& directoryPath, const std::string& filename);
	// 更新
	void Update();
	// 描画
	void Draw();

	// パーティクルグループの生成
	void CreateParticleGroup(const std::string name, const std::string textureFilePath);
	// ルートシグネイチャの作成
	void CreateRootSignature();
	// グラフィックスパイプラインの生成
	void CreateGraphicsPipeline();

	// .mtlファイルの読み込み
	MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	// .objファイルの読み込み
	ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	// パーティクル生成関数
	Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);
	// パーティクルの発生
	void Emit(const std::string& name, const Vector3& position, uint32_t const);

	// シングルトンインスタンスの取得
	static ParticleManager* GetInstance();

	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(ParticleManager&) = delete;
	ParticleManager& operator=(ParticleManager&) = delete;

private:
	// ブレンドモード
	enum BlendMode {
		kBlendModeNone,		// ブレンドなし
		kBlendModeNormal,	// 通常ブレンド
		kBlendModeAdd,		// 加算
		kBlendModeSubtract,	// 減算
		kBlendModeMultiply,	// 乗算
		kBlendModeScreen,	// スクリーン
		kCountOfBlendMode,	// ブレンドモードの数
	};
	BlendMode blendMode = kBlendModeNormal;

	// シングルトンインスタンス
	static std::unique_ptr <ParticleManager> instance;

	// ルートシグネイチャ
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = nullptr;
	D3D12_BLEND_DESC blendDesc{};
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;

	// Objファイルのデータ
	ModelData modelData;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;

	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	Material* materialData = nullptr;

	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// 場の情報
	AccelerationField accelerationField;
	// 場の影響
	bool useField = false;

	// デルタタイム(60fps固定)
	const float kDeltaTime = 1.0f / 60.0f;

	// インスタンス数
	const uint32_t kNumMaxInstance = 100;

	// DirectXCommonのポインタ
	DirectXCommon* dxCommon_ = nullptr;
	// SrvManagerのポインタ
	SrvManager* srvManager_ = nullptr;
	// カメラのポインタ
	Camera* camera_ = nullptr;

};

