#pragma once
#include <D3d12.h>
#include <cassert>
#include <wrl.h>
#include <dxcapi.h>
#include <memory>

#include "Calc.h"

class DirectXCommon;
class Camera;

struct CloudParam {
	Matrix4x4 invViewProj;

	Vector3 cameraPos;
	float time;

	Vector3 sunDir;
	float density;

	float cloudBottom;
	float cloudTop;

	int isRialLight;
	int isAnimeLight;
	int pad[2];

};

class RayMarching {
public:
	// 初期化
	void Initialize(Camera* camera);
	// 更新
	void Update(Camera* camera);
	// 描画
	void Draw();

	// パラメーター
	void SetInvViewProj(Matrix4x4 invViewProj) { cloudParam->invViewProj = invViewProj; }
	void SetTime(float time) { cloudParam->time = time; }
	void SetSunDir(Vector3 sunDir) { cloudParam->sunDir = sunDir; }
	void SetDensity(float density) { cloudParam->density = density; }
	void SetCloudBottom(float cloudBottom) {cloudParam->cloudBottom = cloudBottom;}
	void SetCloudTop(float cloudTop) {cloudParam->cloudTop = cloudTop;}
	void SetRialLight(bool isRialLight){ cloudParam->isRialLight = isRialLight; }
	void SetAnimeLight(bool isAnimeLight){ cloudParam->isAnimeLight = isAnimeLight; }

	// シングルトンインスタンスの取得
	static RayMarching* GetInstance();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> cloudParamResource;
	CloudParam* cloudParam;

	// シングルトンインスタンス
	static std::unique_ptr <RayMarching> instance;

	// ルートシグネイチャ
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[4] = {};
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = nullptr;
	D3D12_BLEND_DESC blendDesc{};
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;

	// DirectXCommonポインタ
	DirectXCommon* dxCommon_ = nullptr;

	// ルートシグネイチャの作成
	void CreateRootSignature();
	// グラフィックスパイプラインの生成
	void CreateGraphicsPipeline();

};

