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
#include "RailCamera.h"
#include "ScoreManager.h"

using namespace DirectX;


class ResultScene : public BaseScene
{

public:
	void Initialize()override;
	void Update()override;
	void Draw2D()override;
	void Draw3D()override;
	void Finalize()override;


private:

	Transform cameraTransform{
	   { 1.0f, 1.0f, 1.0f }, // scale
	   { 0.0f, 0.0f, 0.0f }, // rotate
	   { 0.0f, 0.0f, -5.0f } // translate
	};
	// パーティクル
	Transform transformParticle
	{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};


	// *ライティング* //

	// 平行光
	bool isDirectionalLight = false;
	Vector4 DirectionalLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector3 DirectionalLightDirection = { 0.0f, -1.0f, 0.0f };
	float DirectionalLightIntensity = 1.0f;
	// 環境光
	bool isAmbientLight = true;
	Vector4 AmbientLightColor = { 0.2f, 0.2f, 0.2f,1.0f };
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
	bool isTwoColor = false; // モノクロのON/OFF
	float threshold = 0.5f; // 白と黒の境界値 (0.0~1.0)
	float contrast = 1.0f; // コントラストの強さ

	// 放射線ブラー
	bool isRadialBlur = false;
	Vector2 blurCenter = { 0.5f,0.5f };
	float blurWidth = 0.01f;
	int blurSamples = 10;

	// ディスタンスフォグ
	bool isDistanceFog = false;
	Vector3 distanceFogColor = { 0.5f,0.5f,0.5f };
	float distanceStart = 5.0f;
	float distanceEnd = 20.0f;

	// ハイトフォグ
	bool isHeightFog = false;
	Vector3 heightFogColor = { 0.5f,0.5f,0.5f };
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
	bool isLensFlare = true;           // レンズフレアのON/OFF
	int lensFlareGhostCount = 6;   // ゴーストの数（例: 4～8）
	float lensFlareHaloWidth = 0.57f;      // ヘイロー（輪っか）の大きさ
	bool isACES = true;                 // ACESトーンマッピングをONにする
	float caIntensity = 0.05f;          // 色収差の強さ（最初は弱めに）

	// モーションブラー
	bool isMotionBlur = true;    // モーションブラーのON/OFF
	int motionBlurSamples = 16; // サンプル数（例：8〜16）
	float motionBlurScale = 1.0f;   // ブラーの強さ

	// 色収差
	bool isFullScreenCA = false; // 画面全体の色収差ON/OFF
	float fullScreenCAIntensity = 0.005f; // 画面全体の色収差の強さ
	// 集中線
	bool isConcentrationLines = false; // 集中線のON/OFF
	float concentrationLineIntensity = 1.0f; // 線の濃さ
	Vector2 concentrationLineCenter = { 0.5f, 0.5f };  // 中心座標 (通常 0.5, 0.5)
	float concentrationLineDensity = 1000.0f;   // 線の密度（本数）
	float concentrationLineLength = 0.0f;    // 線の長さ（中心からの開始距離 0.0〜1.0）
	float concentrationLineSpeed = 20.0f;     // 線の動く速さ


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

	// カメラ
	std::unique_ptr<Camera> camera = nullptr;
	// スプライト
	std::unique_ptr <Sprite> sprite = nullptr;

	//数字
	std::unique_ptr<Sprite> numberSprites_[10][5];
	// 3Dオブジェクト
	std::unique_ptr <Object> object[2]{};
	
	//スコア演出

	//クラスのメンバ変数として持っておく
	std::vector<ScoreData> history_;

	float countTimer_ = 0.0f;//2秒計るためのタイマー
	const float kMaxCount_ = 2.0f;//2秒経ったらスコア数字表示

	int currentDigitIndex_ = 0;//今何桁目か(0～4桁)
	int targetScore_ = 0;//実際のスコア
	int actualDigits_[5] = { 0 };//実際のスコアの各桁の数字
	int displayNumbers_[5] = { 0 };//画面に表示させる数字(シャッフル中はランダムな数字が入る)

	//フラグ
	bool isScoreStartTime_ = false;//スコアカウントスタート
	bool isCanPress_ = false;//ボタン押せるようにする
};

