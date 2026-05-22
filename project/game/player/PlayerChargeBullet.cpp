#include "PlayerChargeBullet.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "TrailEffectManager.h"
#include "player.h"
#include <cmath> // sqrt用

// Initializeに必要な引数を追加しています（レティクルの位置、最大距離、寿命）
void PlayerChargeBullet::Initialize(const Vector3& position, Camera* camera, const Vector2 reticlePosition, const float renge, const std::list<std::shared_ptr<Enemy>>& enemies, int style) {
	// --- 1. 基本設定（既存） ---
	transform_.scale = {0.2f, 0.2f, 0.2f};
	transform_.translate = position;
	object_ = std::make_unique<Object>();
	object_->Initialize(camera);
	switch (style) {
	case normal:
		object_->SetModel("normalCBullet.obj");
		break;
	case speed:
		object_->SetModel("speedCBullet.obj");
		break;
	case power:
		object_->SetModel("powerCBullet.obj");
		break;
	case sniper:
		object_->SetModel("sniperCBullet.obj");
		break;
	default:
		object_->SetModel("normalCBullet.obj");
		break;
	}

	camera_ = camera;
	lifeTime_ = 0;
	isActive_ = true;

	// --- 2. ターゲット地点の計算（既存） ---
	float diffX = reticlePosition.x - (WindowAPI::kClientWidth / 2.0f);
	float diffY = (WindowAPI::kClientHeight / 2.0f) - reticlePosition.y;
	float fovY = 0.45f;
	float focalLength = (WindowAPI::kClientHeight / 2.0f) / std::tan(fovY / 2.0f);
	Vector3 targetPosCS = {(diffX / focalLength) * renge, (diffY / focalLength) * renge, renge};
	Matrix4x4 camWorldMat = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, camera_->GetRotate(), camera_->GetTranslate());
	Vector3 targetPosWorld = VectorTransform(targetPosCS, camWorldMat);

	// --- 3. 初速と方向の決定 ---
	Vector3 toTarget = {targetPosWorld.x - position.x, targetPosWorld.y - position.y, targetPosWorld.z - position.z};
	float distToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

	// 弾の速さを固定（時間で割る）
	bulletSpeed_ = distToTarget / (float)maxLifeTime_;

	// 正規化した方向ベクトルを速度に変換
	velocity_ = {(toTarget.x / distToTarget) * bulletSpeed_, (toTarget.y / distToTarget) * bulletSpeed_, (toTarget.z / distToTarget) * bulletSpeed_};

	// hommingAccuracy_ から探索範囲（角度）を算出します
	// ※ hommingAccuracy_ は値が小さい(0.01等)ため、10.0f を掛けて角度を広げています。
	// この 10.0f の部分はお好みの探索範囲に合わせて調整してください！
	float searchAngle = hommingAccuracy_ * 10.0f;
	float minDotThreshold = std::cos(searchAngle);

	float maxDot = -1.0f; // これまでで一番レティクルに近い（内積が大きい）値を記憶する変数
	targetEnemy_.reset(); // ターゲットを一度リセットしておきます

	for (const auto& enemy : enemies) {
		Vector3 enemyPos = enemy->GetWorldPosition();
		Vector3 toEnemy = {enemyPos.x - position.x, enemyPos.y - position.y, enemyPos.z - position.z};
		float distToEnemy = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		// 【仕様5】敵とプレイヤー(発射位置)の距離が射程(renge)より遠い場合は除外
		if (distToEnemy > renge) {
			continue; // この敵はスキップして次の敵をチェックします
		}

		// 弾の進行方向(レティクル方向)と、敵への方向の内積を計算
		Vector3 dirToEnemy = {toEnemy.x / distToEnemy, toEnemy.y / distToEnemy, toEnemy.z / distToEnemy};
		Vector3 currentDir = {velocity_.x / bulletSpeed_, velocity_.y / bulletSpeed_, velocity_.z / bulletSpeed_};
		float dot = currentDir.x * dirToEnemy.x + currentDir.y * dirToEnemy.y + currentDir.z * dirToEnemy.z;

		// 【仕様3】探索範囲内であり、かつ「今まで見つけたどの敵よりもレティクルに近い」場合
		if (dot > minDotThreshold && dot > maxDot) {
			maxDot = dot;         // 最も高い内積の値を更新
			targetEnemy_ = enemy; // ターゲット候補をこの敵に更新
		}
	}

	// トレイルエフェクト
	trailEffect->Initialize("Resource/trail/trail.png", transform_, 3.0f, 3.0f);
	trailEffect->LoadCsv("Resource/trail/chargeShot.csv");
	TrailEffectManager::GetInstance()->AddTrail(trailEffect);

	// ショットエフェクト
	for (int i = 0; i < shotEffectCount; i++) {
		shotEffect[i] = std::make_shared<ParticleEmitter>();
		shotEffect[i]->Initialize("ChargeShot", transform_, 5, 0.1f);
	}
	shotEffect[0]->SetActive("ChargeShot");
	shotEffect[0]->LoadParticle("Resource/particle/shot_1.csv");
	shotEffect[1]->SetActive("ChargeShot2");
	shotEffect[1]->LoadParticle("Resource/particle/shot_2.csv");
	shotEffect[1]->SetTranslate(transform_.translate);
	shotEffect[1]->Emit();

	// 進んでいる方向（velocity_）に合わせて向き（回転）を計算
	float speedXZ = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
	if (speedXZ > 0.0001f || std::abs(velocity_.y) > 0.0001f) {
		transform_.rotate.y = std::atan2(velocity_.x, velocity_.z);
		transform_.rotate.x = std::atan2(-velocity_.y, speedXZ);
	}

	object_->SetScale(transform_.scale);
	object_->SetTranslate(transform_.translate);
	object_->SetRotate(transform_.rotate); // ★追加：回転を適用
}
void PlayerChargeBullet::Update(float cmrvel) {
	std::shared_ptr<Enemy> target = targetEnemy_.lock();

	if (target) {
		// 1. 敵への方向を計算
		Vector3 enemyPos = target->GetWorldPosition();
		Vector3 toEnemy = {enemyPos.x - transform_.translate.x, enemyPos.y - transform_.translate.y, enemyPos.z - transform_.translate.z};
		float dist = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		if (dist > 0.1f) {
			// 敵への理想的な速度ベクトル
			Vector3 targetVelocity = {(toEnemy.x / dist) * bulletSpeed_, (toEnemy.y / dist) * bulletSpeed_, (toEnemy.z / dist) * bulletSpeed_};

			// 2. 現在の速度を理想の速度に近づける (Lerp)
			velocity_.x += (targetVelocity.x - velocity_.x) * hommingAccuracy_;
			velocity_.y += (targetVelocity.y - velocity_.y) * hommingAccuracy_;
			velocity_.z += (targetVelocity.z - velocity_.z) * hommingAccuracy_;

			// 速度の大きさを一定に保つ（曲がっても加速しないように）
			float currentSpeed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
			velocity_.x = (velocity_.x / currentSpeed) * bulletSpeed_;
			velocity_.y = (velocity_.y / currentSpeed) * bulletSpeed_;
			velocity_.z = (velocity_.z / currentSpeed) * bulletSpeed_;
		}
	}

	// 3. 座標更新（共通）
	transform_.translate += velocity_ + Vector3{cmrvel, cmrvel, cmrvel};

	// --- 修正箇所：ここから追加 ---
	// 進んでいる方向（velocity_）に合わせて向き（回転）を計算
	float speedXZ = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
	if (speedXZ > 0.0001f || std::abs(velocity_.y) > 0.0001f) {
		transform_.rotate.y = std::atan2(velocity_.x, velocity_.z);
		transform_.rotate.x = std::atan2(-velocity_.y, speedXZ);
	}

	object_->SetTranslate(transform_.translate);
	object_->SetRotate(transform_.rotate); // ★追加：回転を適用
	object_->Update();

	if (lifeTime_ >= maxLifeTime_) {
		isActive_ = false;
	} else {
		lifeTime_++;
	}

	// トレイルエフェクト更新
	trailEffect->AddPoint(transform_.translate);
	trailEffect->SetTranslate(transform_.translate);
	// ショットエフェクト更新
	for (int i = 0; i < shotEffectCount; i++) {
		shotEffect[i]->SetTranslate(transform_.translate);
		shotEffect[i]->Update();
	}
}
void PlayerChargeBullet::Draw3D() {
	if (isActive_) {
		object_->Draw();
	}
}
void PlayerChargeBullet::Draw2D() {};
