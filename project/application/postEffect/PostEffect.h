#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <dxcapi.h>
#include <memory>

#include "Calc.h"
#include "RayMarching.h"

class DirectXCommon;
class WindowAPI;
class SrvManager;
class Camera;

struct RenderTarget {
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle{};
};

struct EffectData {
	int32_t isInversion;				// 色反転
	int32_t isGrayscale;				// グレースケール
	int32_t isRadialBlur;				// 放射線ブラー
	int32_t isDistanceFog;				// ディスタンスフォグ

	int32_t isDoF;						// 被写界深度
	int32_t isHeightFog;				// ハイトフォグ
	float intensity;					// 効果の強さ
	float pad0;

	// 放射線ブラー
	Vector2 blurCenter;					// ブラーの中心
	float blurWidth;					// ブラーの幅
	int32_t blurSamples;				// ブラーのサンプル数

	// ディスタンスフォグ
	Vector3 distanceFogColor;			// フォグの色
	float distanceFogStart;				// フォグが始まる距離

	float distanceFogEnd;				// 完全にフォグに覆われる距離
	float zNear;						// カメラのニアクリップ面
	float zFar;							// カメラのファークリップ面
	float pad1;

	// ハイトフォグ
	Vector3 heightFogColor;				// ハイトフォグの色
	float heightFogTop;					// ハイトフォグが始まる高さ

	float heightFogBottom;				// ハイトフォグが終わる高さ
	float heightFogDensity;				// ハイトフォグの密度
	Vector2 pad2;

	Matrix4x4 matInverseViewProjection; // ハイトフォグ用の逆行列

	// 被写界深度
	float focusDistance;				// DOFのピントが合う距離
	float focusRange;					// DOFのピントが合う範囲（遊び）
	float bokehRadius;					// DOFのボケの最大半径
	float pad3;

	// ブルーム
	float bloomThreshold;				// 輝度の閾値
	float bloomIntensity;				// ブルームの強さ
	float bloomBlurRadius;				// ブルームのぼかし半径
	float pad4;

	// レンズフレア
	int32_t isLensFlare;				// レンズフレアのON/OFF
	int32_t lensFlareGhostCount;		// ゴーストの数
	float lensFlareGhostDispersal;		// ゴーストの分散
	float lensFlareHaloWidth;			// ハローの幅

	// ACESトーンマッピング
	int32_t isACES;						// ON/OFF
	float caIntensity;					// 色収差の強さ
	Vector2 pad5;

	// モーションブラー
	int32_t isMotionBlur;				// モーションブラーのON/OFF
	int32_t motionBlurSamples;			// モーションブラーのサンプル数（例：8〜16）
	float motionBlurScale;				// モーションブラーの強さ
	float pad6;

	// 色収差
	int isFullScreenCA;					// 画面全体の色収差ON/OFF
	float fullScreenCAIntensity;		// 画面全体の色収差の強さ
	// ビネット
	int isVignette;						// ビネットON/OFF
	float vignetteIntensity;			// ビネットの強さ

	// スピードディストーション
	int isSpeedDistortion;				// スピードディストーションのON/OFF
	float speedDistortionStrength;		// 歪みの強さ
	Vector2 pad7;

	// 集中線
	int isConcentrationLines;			// ON/OFF
	float concentrationLineIntensity;	// 線の濃さ
	Vector2 concentrationLineCenter;	// 中心座標 (通常 0.5, 0.5)

	float concentrationLineDensity;		// 線の密度（本数）
	float concentrationLineLength;		// 線の長さ（中心からの開始距離 0.0〜1.0）
	float concentrationLineSpeed;		// アニメーション速度
	float time;							// アニメーション用の時間

	// ピンチエフェクト
	int32_t isPinch;					// ピンチエフェクトのON/OFF
	float pinchStrength;				// 歪みの強さ（正の値で吸い込み、負の値で膨張）
	Vector2 pinchCenter;				// 歪みの中心 (通常 0.5, 0.5)

	float pinchRadius;					// 歪みが影響する半径
	Vector3 pad8;

	// 二値化
	int32_t isTwoColor;
	float threshold;					// 白と黒の境界値 (0.0~1.0)
	float contrast;						// コントラストの強さ
	float pad9;

};


// 各パスのレンダーターゲットとSRVインデックスをまとめる構造体
struct BloomBuffer {
	RenderTarget lumRenderTarget;
	uint32_t lumSrvIndex;

	RenderTarget blurRenderTarget[2];
	uint32_t blurSrvIndex[2];
};

class PostEffect {
public:
	// 初期化
	void Initialize(DirectXCommon* dxCommon, WindowAPI* windowAPI, SrvManager* srvManager);
	// 更新
	void Update(Camera* camera);
	// 描画
	void Draw();

	// 描画前処理
	void PreDraw();
	// 描画後処理
	void PostDraw();

	// 反転
	void SetInversion(bool isInversion) { effectData->isInversion = isInversion; }
	// グレースケール
	void SetGrayscale(bool isGrayscale) { effectData->isGrayscale = isGrayscale; }
	void SetTwoColor(bool isTwoColor) { effectData->isTwoColor = isTwoColor; }
	void SetThreshold(float threshold) { effectData->threshold = threshold; }
	void SetContrast(float contrast) { effectData->contrast = contrast; }
	// 放射線ブラー
	void SetRadialBlur(bool isRadialBlur) { effectData->isRadialBlur = isRadialBlur; }
	void SetBlurCenter(const Vector2& center) { effectData->blurCenter = center; }
	void SetBlurWidth(float width) { effectData->blurWidth = width; }
	void SetBlurSamples(int samples) { effectData->blurSamples = samples; }

	// ディスタンスフォグ
	void SetDistanceFog(bool isFog) { effectData->isDistanceFog = isFog; }
	void SetDistanceFogColor(const Vector3& color) { effectData->distanceFogColor = color; }
	void SetDistanceFogStart(float start) { effectData->distanceFogStart = start; }
	void SetDistanceFogEnd(float end) { effectData->distanceFogEnd = end; }
	
	// ハイトフォグ
	void SetHeightFog(bool isFog) { effectData->isHeightFog = isFog; }
	void SetHeightFogColor(const Vector3& color) { effectData->heightFogColor = color; }
	void SetHeightFogTop(float top) { effectData->heightFogTop = top; }
	void SetHeightFogBottom(float bottom) { effectData->heightFogBottom = bottom; }
	void SetHeightFogDensity(float density) { effectData->heightFogDensity = density; }
	void SetInverseViewProjectionMatrix(const Matrix4x4& mat) { effectData->matInverseViewProjection = mat; }
	void HightFogUpdate(Camera* camera); // カメラの位置からハイトフォグ用の逆行列を計算してセットする関数

	// DOF
	void SetDOF(bool isDoF) { effectData->isDoF = isDoF; }
	void SetFocusDistance(float distance) { effectData->focusDistance = distance; }
	void SetFocusRange(float range) { effectData->focusRange = range; }
	void SetBokehRadius(float radius) { effectData->bokehRadius = radius; }

	// ブルーム
	void SetBloomThreshold(float bloomThreshold) {effectData->bloomThreshold = bloomThreshold;}
	void SetBloomIntensity(float bloomIntensity) {effectData->bloomIntensity = bloomIntensity;}
	void SetBloomBlurRadius(float bloomBlurRadius) { effectData->bloomBlurRadius = bloomBlurRadius; }
	// レンズフレア
	void SetLensFlare(bool isLensFlare) { effectData->isLensFlare = isLensFlare; }
	void SetLensFlareGhostCount(int count) { effectData->lensFlareGhostCount = count; }
	void SetLensFlareGhostDispersal(float dispersal) { effectData->lensFlareGhostDispersal = dispersal; }
	void SetLensFlareHaloWidth(float width) { effectData->lensFlareHaloWidth = width; }
	void SetCAIntensity(float intensity) { effectData->caIntensity = intensity; } // 色収差
	void SetIsACES(bool isACES) { effectData->isACES = isACES; } // ACES
	// モーションブラー
	void SetMotionBlur(bool isMotionBlur) { effectData->isMotionBlur = isMotionBlur; }
	void SetMotionBlurSamples(int motionBlurSamples) { effectData->motionBlurSamples = motionBlurSamples;}
	void SetMotionBlurScale(float motionBlurScale) {effectData->motionBlurScale = motionBlurScale;}
	// 色収差
	void SetFullScreenCA(bool isFullScreenCA) { effectData->isFullScreenCA = isFullScreenCA; }
	void SetFullScreenCAIntensity(float intensity) { effectData->fullScreenCAIntensity = intensity; }
	// ビネット
	void SetVignette(bool isVignette) { effectData->isVignette = isVignette; }
	void SetVignetteIntensity(float intensity) { effectData->vignetteIntensity = intensity; }
	// ダメージエフェクト
	void SetDamageEffectRatio(float ratio) { damageEffectRatio_ = ratio; }
	// スピードディストーション
	void SetSpeedDistortion(bool isSpeedDistortion) { effectData->isSpeedDistortion = isSpeedDistortion; }
	void SetSpeedDistortionStrength(float strength) { effectData->speedDistortionStrength = strength; }
	// 集中線
	void SetConcentrationLines(bool isConcentrationLines) { effectData->isConcentrationLines = isConcentrationLines; }
	void SetConcentrationLineIntensity(float intensity) { effectData->concentrationLineIntensity = intensity; }
	void SetConcentrationLineCenter(const Vector2& center) { effectData->concentrationLineCenter = center; }
	void SetConcentrationLineDensity(float density) { effectData->concentrationLineDensity = density; }
	void SetConcentrationLineLength(float length) { effectData->concentrationLineLength = length; }
	void SetConcentrationLineSpeed(float speed) { effectData->concentrationLineSpeed = speed; }
	void SetTime(float time) { effectData->time = time; }
	// ピンチエフェクト
	void SetPinch(bool isPinch) { effectData->isPinch = isPinch; }
	void SetPinchStrength(float strength) { effectData->pinchStrength = strength; }
	void SetPinchCenter(const Vector2& center) { effectData->pinchCenter = center; }
	void SetPinchRadius(float radius) { effectData->pinchRadius = radius; }
	// エフェクトの強さ
	void SetIntensity(float intensity) { effectData->intensity = intensity; }

	// getter
	float GetLensFlareGhostDispersal() { return effectData->lensFlareGhostDispersal; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(uint32_t index) { return rtvHandles[index]; }
	
	// シングルトンインスタンスの取得
	static PostEffect* GetInstance();

	// 深度バッファを指定の状態に遷移
	void TransitionDepthBuffer(D3D12_RESOURCE_STATES newState);

private:
	// ルートシグネイチャ
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = nullptr;

	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateFinal;

	// レンダーターゲット
	RenderTarget renderTarget_;
	RenderTarget lumRenderTarget_;     // 高輝度抽出用
	RenderTarget blurRenderTarget_[2]; // ぼかし用（Ping-Pong処理用）
	RenderTarget lensFlareRenderTarget_; // レンズフレア
	RenderTarget velocityRenderTarget_; // ベロシティ
	// ビューポート
	D3D12_VIEWPORT viewport_;
	// シザー矩形
	D3D12_RECT scissorRect_;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	// クリアカラー
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	D3D12_RESOURCE_STATES currentState_;

	// *エフェクト切り換え用* //
	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> effectResource;
	// エフェクトデータ
	EffectData* effectData = nullptr;

	// index
	uint32_t srvIndex_;
	// PostEffect.h の private メンバ変数などに以下を追加
	uint32_t depthSrvIndex_ = 0;

	// シングルトンインスタンス
	static std::unique_ptr <PostEffect> instance;

	// ブルームのパス数（1/2, 1/4, 1/8 の3段階）
	static const int kBloomPassCount = 3;
	// 3段階分のバッファ配列
	BloomBuffer bloomBuffers_[kBloomPassCount];

	// レンズフレア
	uint32_t lensFlareSrvIndex_ = 0;
	// モーションブラー
	uint32_t velocitySrvIndex_ = 0; // ベロシティバッファ(t6)用
	// ダメージエフェクト
	bool isDamegeFade = true; // フェードアウト
	float damageEffectRatio_ = 0.0f; // 0.0fでエフェクト無し、1.0fで最大ダメージ表現

	// ポインター
	DirectXCommon* dxCommon_ = nullptr;
	WindowAPI* windowAPI_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// レンダーターゲットの生成
	RenderTarget CreateRenderTarget(
		ID3D12Device* device,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		const float clearColor[4],
		ID3D12DescriptorHeap* rtvHeap,
		UINT rtvIndex,
		ID3D12DescriptorHeap* srvHeap,
		UINT srvIndex
	);

	// リソースバリアの発行
	void Transition(D3D12_RESOURCE_STATES newState);
	// バックバッファを指定の状態に遷移
	void TransitionBackBuffer(D3D12_RESOURCE_STATES newState);

	// リソースの状態を切り替える関数
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	// ルートシグネイチャ生成
	void CreateRootSignature();
	// グラフィックスパイプライン生成
	void CreateGraphicsPipeline();

	// ビューポート初期化
	void InitializeViewport();
	// シザリング矩形初期化
	void InitializeScissorRect();
};

