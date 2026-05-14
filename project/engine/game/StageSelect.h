#pragma once
#include <DirectXMath.h>

#include "BaseScene.h"
#include "Camera.h"
#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "ModelManager.h"
#include "Object.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "PostEffect.h"
#include "RailCamera.h"
#include "SoundManager.h"
#include "Sprite.h"
#include "game/player/Player.h"
#include "Easing.h"
#include "Book.h"
#include "RadarChart.h"


using namespace DirectX;

class SpriteCommon;
class ObjectCommon;

class StageSelect : public BaseScene {
public:
	void Initialize() override;
	void Update() override;
	void Draw2D() override;
	void Draw3D() override;

	void Finalize() override;
	int GetPlayerStyle() override { return currentStyle; };
	int GetCurrentStage() override { return currentStage; };

private:
	Transform transform_;

	Transform cameraTransform{
		{1.0f, 1.0f, 1.0f }, // scale
		{0.0f, 0.0f, 0.0f }, // rotate
		{0.0f, 0.0f, -5.0f}  // translate
	};
	// パーティクル
	Transform transformParticle{
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	// カメラ
	std::unique_ptr<Camera> camera_ = nullptr;
	// パーティクルエミッタ
	std::unique_ptr<ParticleEmitter> particleEmitter = nullptr;

	Style currentStyle = normal;

	bool isStageSelect = false;
	int currentStage = 1;

	// イージング
	std::unique_ptr <Easing> easing;

	// 平行光
	bool isDirectionalLight = false;
	Vector4 DirectionalLightColor = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 DirectionalLightDirection = {0.0f, -1.0f, 0.0f};
	float DirectionalLightIntensity = 1.0f;
	// 環境光
	bool isAmbientLight = true;
	Vector4 AmbientLightColor = {0.2f, 0.2f, 0.2f};
	float AmbientLightIntensity = 1.0f;
	// ポイントライト
	bool isPointLight = false;
	Vector4 PointLightColor = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 PointLightPosition = {1.0f, 1.0f, 0.0f};
	float PointLightIntensity = 1.0f;
	// スポットライト
	bool isSpotLight = false;
	Vector4 SpotLightColor = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 SpotLightPosition = {0.0f, 0.0f, 0.0f};
	Vector3 SpotLightDirection = {0.0f, 0.0f, 0.0f};
	float SpotLightRange = 10.0f;
	float SpotLightIntensity = 1.0f;

	// *ポストエフェクト* //

	// 反転
	bool isInversion = false;
	// グレースケール
	bool isGrayscale = false;

	// 放射線ブラー
	bool isRadialBlur = false;
	Vector2 blurCenter = {0.5f, 0.5f};
	float blurWidth = 0.0f;
	int blurSamples = 10;

	// ディスタンスフォグ
	bool isDistanceFog = false;
	Vector3 distanceFogColor = {0.5f, 0.5f, 0.5f};
	float distanceStart = 5.0f;
	float distanceEnd = 20.0f;

	// ハイトフォグ
	bool isHeightFog = false;
	Vector3 heightFogColor = {0.5f, 0.5f, 0.5f};
	float heightFogTop = 0.0f;
	float heightFogBottom = -5.0f;
	float heightFogDensity = 1.0f;

	// DOF
	bool isDOF = false;
	float focusDistance = 5.0f;
	float focusRange = 5.0f;
	float bokehRadius = 5.0f;

	// ブルーム
	float bloomThreshold = 1.5f;
	float bloomIntensity = 0.0f;
	float bloomBlurRadius = 1.0f;

	// レンズフレア
	bool isLensFlare = true;          // レンズフレアのON/OFF
	int lensFlareGhostCount = 6;      // ゴーストの数（例: 4～8）
	float lensFlareHaloWidth = 0.57f; // ヘイロー（輪っか）の大きさ
	bool isACES = true;               // ACESトーンマッピングをONにする
	float caIntensity = 0.05f;        // 色収差の強さ（最初は弱めに）

	// モーションブラー
	bool isMotionBlur = true;     // モーションブラーのON/OFF
	int motionBlurSamples = 16;   // サンプル数（例：8〜16）
	float motionBlurScale = 1.0f; // ブラーの強さ

	// 色収差
	bool isFullScreenCA = false; // 画面全体の色収差ON/OFF
	float fullScreenCAIntensity = 0.0f; // 画面全体の色収差の強さ

	// スピードディストーション
	bool isSpeedDistortion = false; // スピードディストーションのON/OFF
	float speedDistortionStrength = 0.0f; // 歪みの強さ

	float intensity = 0.0f; // エフェクト全体の強さ

	// レイマーチング
	// float rayMarchingTime = 0.0f; ;
	Vector3 rayMarchingSunDir = {0.07f, -0.17f, -0.75f};
	float rayMarchingCloudCoverage = 0.22f;
	float rayMarchingCloudBottom = 70.0f;
	float rayMarchingCloudTop = -300.0f;
	bool rayMarchingIsRialLight = false;
	bool rayMarchingIsAnimeLight = true;
	bool rayMarchingIsMotionBlur = false;
	float rayMarchingCloudOpacity = 0.01f;

	//スプライト
	std::unique_ptr<Sprite> return_ = nullptr;
	std::unique_ptr<Sprite> enter_ = nullptr;


	// 本型UI
	std::unique_ptr <Book> book = nullptr;
	EasingSet bookEasing;

	// レーダーチャート
	std::unique_ptr <RadarChart> radarChart = nullptr;
	std::vector<float> values = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
	Vector2 radarPosition = { 1247.0f, 530.0f };
	float radarChartRadius = 204.0f;
	EasingSet radarChartEasing[6];
	int kMaxRadarChart = 6;
	DirectX::XMFLOAT4 radarChartColor = { 0.0f, 1.0f, 1.0f, 0.5f };

	// 切り換えクールタイム
	float switchCooltime = 0.0f;

	float parameterSetting[6][4] = {
		{0.88f, 0.2f, 1.44f, 0.8f},	// 体力
		{0.5f, 0.16f, 4.0f, 0.4f},	// 攻撃力
		{0.6f, 1.2f, 0.1f, 0.02f},	// チャージ速度
		{0.59f, 0.1f, 0.54f, 0.0f},	// ホーミング精度
		{0.5f, 1.5f, 0.2f, 0.1f},	// 連射速度
		{0.5f, 0.3f, 0.5f, 2.0f},	// チャージ攻撃力
		//Normal	Speed	Power	Sniper
	};
	bool isParameterEasing = false;

	// セレクト->ゲーム
	bool isTransition = false;
	// セレクト->タイトル
	bool isBackTransition = false;

	void TransitionUpdate();

	void LithingEffect();

	// パラメータのイージングを設定
	void ParameterEasingSet(Style currentStyle);
};
