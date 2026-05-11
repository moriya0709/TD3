#pragma once
#include <DirectXMath.h>

#include "Camera.h"
#include "Sprite.h"
#include "Object.h"
#include "ParticleEmitter.h"
#include "CameraManager.h"
#include "ParticleManager.h"
#include "ModelManager.h"
#include "SoundManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "BaseScene.h"
#include "PostEffect.h"
#include "Book.h"
#include "RadarChart.h"

using namespace DirectX;

class SpriteCommon;
class ObjectCommon;

class TitleScene : public BaseScene {
public:
	// 初期化
	void Initialize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw2D() override;
	void Draw3D() override;
	// 終了
	void Finalize() override;

private:
	Transform cameraTransform{
	   { 1.0f, 1.0f, 1.0f }, // scale
	   { 0.0f, 0.0f, 0.0f }, // rotate
	   { 0.0f, 1.0f, -5.0f } // translate
	};
	// パーティクル
	Transform transformParticle
	{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{10.0f,0.0f,0.0f}
	};


	// *ライティング* //

	// 平行光
	bool isDirectionalLight = false;
	Vector4 DirectionalLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector3 DirectionalLightDirection = { 0.0f, -1.0f, 0.0f };
	float DirectionalLightIntensity = 1.0f;
	// 環境光
	bool isAmbientLight = true;
	Vector4 AmbientLightColor = { 0.2f, 0.2f, 0.2f,1.0f};
	float AmbientLightIntensity = 1.0f;
	// ポイントライト
	bool isPointLight = false;
	Vector4 PointLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector3 PointLightPosition = { 1.0f, 1.0f, 0.0f };
	float PointLightIntensity = 1.0f;
	// スポットライト
	bool isSpotLight = false;
	Vector4 SpotLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector3 SpotLightPosition = { 0.0f, 0.0f, 0.0f };
	Vector3 SpotLightDirection = { 0.0f, 0.0f, 0.0f };
	float SpotLightRange = 10.0f;
	float SpotLightIntensity = 1.0f;

	// *ポストエフェクト* //

	// 反転
	bool isInversion = false;
	// グレースケール
	bool isGrayscale = false;


	// 放射線ブラー
	bool isRadialBlur = false;
	Vector2 blurCenter = { 0.5f,0.5f };
	float blurWidth = 0.01f;
	int blurSamples = 10;

	// ディスタンスフォグ
	bool isDistanceFog = false;
	Vector3 distanceFogColor = { 0.5f,0.5f,0.5f};
	float distanceStart = 5.0f;
	float distanceEnd = 20.0f;

	// ハイトフォグ
	bool isHeightFog = false;
	Vector3 heightFogColor = { 0.5f,0.5f,0.5f};
	float heightFogTop = 0.0f;
	float heightFogBottom = -5.0f;
	float heightFogDensity = 1.0f;

	// DOF
	bool isDOF = false;
	float focusDistance = 5.0f;
	float focusRange = 5.0f;
	float bokehRadius = 5.0f;

	// ブルーム
	float bloomThreshold = 1.0f;
	float bloomIntensity = 2.0f;
	float bloomBlurRadius = 1.0f;

	// レンズフレア
	bool isLensFlare = false;           // レンズフレアのON/OFF
	int lensFlareGhostCount = 6;   // ゴーストの数（例: 4～8）
	float lensFlareHaloWidth = 0.57f;      // ヘイロー（輪っか）の大きさ
	bool isACES = true;                 // ACESトーンマッピングをONにする
	float caIntensity = 0.05f;          // 色収差の強さ（最初は弱めに）

	// モーションブラー
	bool isMotionBlur = false;    // モーションブラーのON/OFF
	int motionBlurSamples = 16; // サンプル数（例：8〜16）
	float motionBlurScale = 1.0f;   // ブラーの強さ

	// 色収差
	bool isFullScreenCA = false; // 画面全体の色収差ON/OFF
	float fullScreenCAIntensity = 0.005f; // 画面全体の色収差の強さ

	// レイマーチング
	//float rayMarchingTime = 0.0f; ;
	Vector3 rayMarchingSunDir = { 0.3f, -0.5f, 0.2f };
	float rayMarchingCloudCoverage = 0.22f;
	float rayMarchingCloudBottom = 70.0f;
	float rayMarchingCloudTop = -300.0f;
	bool rayMarchingIsRialLight = false;
	bool rayMarchingIsAnimeLight = true;
	bool  rayMarchingIsMotionBlur = false;
	float  rayMarchingCloudOpacity = 0.04f;
	bool isStorm = false;
	float thunderFrequency = 0.3f;
	float thunderBrightness = 120.0f;

	int padX;
	int padY;

	// ヒットエフェクト
	std::unique_ptr <ParticleEmitter> hitEffect[4] = {};
	const int hitEffectCount = 4; // ヒットエフェクトの数
	
	// カメラ
	std::unique_ptr<Camera> camera = nullptr;
	// スプライト
	std::unique_ptr <Sprite> sprite = nullptr;
	// タイトル
	std::unique_ptr <Sprite> title_ = nullptr;

	// 3Dオブジェクト
	std::unique_ptr <Object> object[2]{};

	std::unique_ptr <ParticleEmitter> particleEmitter = nullptr;

	// 本型UI
	std::unique_ptr <Book> book = nullptr;

	// Updateで直接操作したい特定のオブジェクトへのポインタ(アニメーションモデル)
	Object* walkAnimation = nullptr;

	Skeleton skeleton_;
	Skeleton skinCluster_;

	std::vector<std::unique_ptr<Object>> normalObjects;//通常モデル  
	std::vector<std::unique_ptr<Object>> animationObjects;//アニメーションモデル 

	Animation simpleAnimation_;//スケルトン
	Animation walkAnimation_;//歩きモーション

	// レーダーチャート
	std::unique_ptr <RadarChart> radarChart = nullptr;
	float values[5] = { 0.8f, 0.6f, 0.9f, 0.5f, 0.7f };
	Vector2 radarPosition = { 960.0f, 540.0f };
};