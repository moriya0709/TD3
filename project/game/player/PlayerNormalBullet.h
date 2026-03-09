#pragma once
#include "PlayerBullet.h"
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
class PlayerNormalBullet : public PlayerBullet {
public:
	void Initialize(const Vector3& position, Camera* camera, const Vector2 reticlePosition, const float renge) override;
	void Update() override;
	void Draw3D() override;
	void Draw2D() override;
	void SetStatus(const float hommingAccuracy) override { hommingAccuracy_ = hommingAccuracy; }
	bool IsActive() override { return isActive_; }

private:
	// プレイヤーの弾のステータス
	Transform transform_;
	std::unique_ptr<Object> object_;
	float hommingAccuracy_ = 0.0f;           // ホーミング精度
	float renge_ = 0.0f;                     // 弾速
	Vector2 reticlePosition_ = {0.0f, 0.0f}; // 照準の座標
	Camera* camera_ = nullptr;
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	int lifeTime_ = 0;     // 弾の寿命（フレーム数）
	int maxLifeTime_ = 30; // 弾の最大寿命（フレーム数）
	bool isActive_ = true; // 弾が有効かどうか
};

