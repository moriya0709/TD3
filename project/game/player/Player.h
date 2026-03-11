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
#include <list>

// ベクトルを回転行列によって変換する関数
class Player {
public:
	struct Statas {
		int hp;                       // 体力
		int attack;                   // 攻撃力
		float speed;                  // 速度
		float hommingAccuracy = 0.0f; // ホーミング精度
		float renge = 0.0f;           // 弾速
		int chargeTime = 0;           // チャージ時間
		int haste = 0;                // 攻撃頻度
	};
	void Initialize(Camera* camera);
	void Update();
	void Draw2D();
	void Draw3D();
	Vector3 GetPosition() const { return transform_.translate; }
	~Player();
	float GetHitSize() const { return hitSize_; }
	void Damage(int damage) {
		statas_.hp -= damage;
		if (statas_.hp < 0) {
			statas_.hp = 0;
		}
		ishit = true;
		damageTimer = 30; // ダメージ表示タイマーリセット
	}
	bool GetIsHit() const { return ishit; }

	const std::list<std::unique_ptr<PlayerBullet>>& GetBullets() const { return bullets; }

private:
	// プレイヤーのステータス
	Statas statas_;
	// プレイヤーの座標や回転などの変換情報
	Transform transform_;
	// プレイヤーの3Dオブジェクト
	std::unique_ptr<Object> playerObject_;
	// プレイヤーの2Dスプライト（照準）
	std::unique_ptr<Sprite> reticle_;
	// 照準の座標
	Vector2 reticlePosition_ = {0.0f, 0.0f};
	// カメラのポインタ
	Camera* camera_ = nullptr;
	// プレイヤーの現在の速度
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	Vector3 relativePos_ = {0.0f, 0.0f, 0.0f}; // カメラからの相対位置（Zは固定）
	float hitSize_ = 1.0f;                     // 当たり判定のサイズ

	// プレイヤーの弾
	std::list<std::unique_ptr<PlayerBullet>> bullets;
	void Attack();
	void UpdateBullets();
	// 次の発射まで
	int coolTime = 0;
	bool isSpecialAttack = false;
	int chargeTimer = 0;
	bool isCharging = false;
	bool ishit = false;
	int damageTimer = 0;

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

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);