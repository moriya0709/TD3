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

	// プレイヤーの3Dオブジェクト
	std::unique_ptr<Object> playerObject_;

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

	// パラメータ
	std::unique_ptr<Sprite> parameter[5] = {};
	std::unique_ptr<Sprite> parameterGauge[5] = {};
	EasingSet parameterGaugeEasing[5];
	int kMaxParameter = 5;

	int parameterSetting[5][4] = {
		{80, 50, 10, 30},	// hp
		{70, 60, 70, 80},	// 威力
		{90, 40, 20, 30},	// 連射速度
		{60, 70, 50, 10},	// 移動速度
		{50, 80, 40, 30}	// 射程orチャージ速度
		//Normal	Speed	Power	Sniper
	};


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
	float blurWidth = 0.01f;
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
	float bloomIntensity = 1.0f;
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

	void LithingEffect();

	// パラメータのイージングを設定
	void ParameterEasingSet(Style currentStyle);
};
