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
#include "RayMarching.h"

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
	Vector4 AmbientLightColor = { 0.2f, 0.2f, 0.2f };
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

	// レイマーチング
	//float rayMarchingTime = 0.0f; ;
	Vector3 rayMarchingSunDir = { 0.3f, -0.5f, 0.2f };
	float rayMarchingCloudCoverage = 0.15f;
	float rayMarchingCloudBottom = 50.0f;
	float rayMarchingCloudTop = -240.0f;
	bool rayMarchingIsRialLight = false;
	bool rayMarchingIsAnimeLight = true;

	int padX;
	int padY;
	

	// スプライト
	std::unique_ptr <Sprite> sprite = nullptr;
	// 3Dオブジェクト
	std::unique_ptr <Object> object[2]{};
	// パーティクルエミッタ
	std::unique_ptr <ParticleEmitter> particleEmitter = nullptr;
	// レールカメラ
	std::unique_ptr <RailCamera> railCamera = nullptr;

};