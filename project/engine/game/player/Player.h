#pragma once
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
#include "SoundManager.h"
#include "Sprite.h"

class Player {
public:
	struct Statas {
		int hp;                       // 体力
		int attack;                   // 攻撃力
		float speed;                  // 速度
		float hommingAccuracy = 0.0f; // ホーミング精度
		float bulletSpeed = 0.0f;     // 弾速
		float chargeTime = 0.0f;      // チャージ時間
	};
	void Initialize(Camera* camera);
	void Update();
	void Draw2D();
	void Draw3D();

private:
	Statas statas_;
	Transform transform_;
	std::unique_ptr<Object> object_;
	Camera* camera_ = nullptr;
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f}; // 現在の速度（初期値は0）
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
};
