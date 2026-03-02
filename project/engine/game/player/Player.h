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
};
