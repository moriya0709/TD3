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
#include "../enemy/Enemy.h"

class PlayerBullet {
public:
	virtual void Initialize(const Vector3& position, Camera* camera, const Vector2 reticlePosition, const float renge, const std::list<std::shared_ptr<Enemy>>& enemies);
	virtual void Update(float cmrvel);
	virtual void Draw3D();
	virtual void Draw2D();
	virtual void SetStatus(const float hommingAccuracy,const int damage);
	virtual bool IsActive();
	virtual void SetActive(bool active) {};
	virtual int GetPenetration();
	virtual Vector3 GetPosition() const { return Vector3(); };
	virtual float GetHitSize() const { return 0.0f; };
	virtual int GetDamage() const { return 0; };
	virtual Vector3 GetVelocity() const { return Vector3(); };


private:
	// プレイヤーの弾のステータス
	//Transform transform_;
	//std::unique_ptr<Object> object_;
	//float hommingAccuracy_ = 0.0f; // ホーミング精度
	//float renge_ = 0.0f;           // 弾速
	//Vector2 reticlePosition_ = {0.0f, 0.0f}; // 照準の座標
	//Camera* camera_ = nullptr;
	//Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	//int lifeTime_ = 0; // 弾の寿命（フレーム数）
	//int maxLifeTime_ = 30; // 弾の最大寿命（フレーム数）
	//bool isActive_ = true; // 弾が有効かどうか
};
