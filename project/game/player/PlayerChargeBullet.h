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
#include "PlayerBullet.h"
#include "PostEffect.h"
#include "SoundManager.h"
#include "Sprite.h"

class PlayerChargeBullet : public PlayerBullet {
public:
	void Initialize(const Vector3& position, Camera* camera, const Vector2 reticlePosition, const float renge, const std::list<std::shared_ptr<Enemy>>& enemies) override;
	void Update(float cmrvel) override;
	void Draw3D() override;
	void Draw2D() override;
	void SetStatus(const float hommingAccuracy, const int damage) override {
		hommingAccuracy_ = hommingAccuracy;
		damage_ = damage*2;
	}
	bool IsActive() override { return isActive_; }
	void SetActive(bool active) override { isActive_ = active; }
	int GetPenetration() override { return 0; }
	Vector3 GetPosition() const override { return transform_.translate; }
	float GetHitSize() const override { return 0.5f; } // 例: ヒットサイズ0.5
	virtual int GetDamage() const override { return damage_; }
	Vector3 GetVelocity() const override { return velocity_; }

private:
	// プレイヤーの弾のステータス
	Transform transform_;
	std::unique_ptr<Object> object_;
	float hommingAccuracy_ = 0.0f;           // ホーミング精度
	float renge_ = 0.0f;                     // 弾速
	Vector2 reticlePosition_ = {0.0f, 0.0f}; // 照準の座標
	Camera* camera_ = nullptr;
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	int lifeTime_ = 0;                 // 弾の寿命（フレーム数）
	int maxLifeTime_ = 60;             // 弾の最大寿命（フレーム数）
	bool isActive_ = true;             // 弾が有効かどうか
	std::weak_ptr<Enemy> targetEnemy_; // ホーミング対象の敵
	float bulletSpeed_;                // 弾の速さ
	int damage_ = 0;                   // 弾のダメージ量
};