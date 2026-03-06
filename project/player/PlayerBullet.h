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

class PlayerBullet {
public:
	void Initialize(const Vector3& position, Camera* camera);
	void Update();
	void Draw3D();
	void Draw2D();
	void SetStatus(const float hommingAccuracy, const float renge,const Vector2 reticlePosition) {
		hommingAccuracy_ = hommingAccuracy;
		renge_ = renge;
		reticlePosition_ = reticlePosition;
	}

private:
	// プレイヤーの弾のステータス
	Transform transform_;
	std::unique_ptr<Object> object_;
	float hommingAccuracy_ = 0.0f; // ホーミング精度
	float renge_ = 0.0f;           // 弾速
	Vector2 reticlePosition_ = {0.0f, 0.0f}; // 照準の座標
};
