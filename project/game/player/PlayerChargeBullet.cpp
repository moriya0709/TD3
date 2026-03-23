#include "ObjectCommon.h"
#include "PlayerChargeBullet.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "player.h"
#include <cmath> // sqrt用

// Initializeに必要な引数を追加しています（レティクルの位置、最大距離、寿命）
void PlayerChargeBullet::Initialize(const Vector3& position, Camera* camera, const Vector2 reticlePosition, const float renge, const std::list<std::shared_ptr<Enemy>>& enemies) {
	// --- 1. 基本設定（既存） ---
	transform_.scale = {0.2f, 0.2f, 0.2f};
	transform_.translate = position;
	object_ = std::make_unique<Object>();
	object_->Initialize(camera);
	object_->SetModel("player.obj");
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

	// --- 4. ホーミング対象の探索 ---

	float minAngle = 0.5f; // 探索範囲（ラジアン。約30度以内など）

	for (const auto& enemy : enemies) {
		Vector3 enemyPos = enemy->GetWorldPosition();
		Vector3 toEnemy = {enemyPos.x - position.x, enemyPos.y - position.y, enemyPos.z - position.z};
		float distToEnemy = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		// 弾の進行方向と敵への方向の内積から角度を計算
		Vector3 dirToEnemy = {toEnemy.x / distToEnemy, toEnemy.y / distToEnemy, toEnemy.z / distToEnemy};
		Vector3 currentDir = {velocity_.x / bulletSpeed_, velocity_.y / bulletSpeed_, velocity_.z / bulletSpeed_};

		float dot = currentDir.x * dirToEnemy.x + currentDir.y * dirToEnemy.y + currentDir.z * dirToEnemy.z;

		// 内積が1に近いほど方向が一致している。一定範囲内かつ一番近い敵を選ぶ等の処理
		if (dot > std::cos(minAngle)) {
			targetEnemy_ = enemy; // 修正: unique_ptr<Enemy> ではなく Enemy* 型にする
			// 最初に見つかった敵、あるいは一番近い敵をセット
			break;
		}
	}

	object_->SetScale(transform_.scale);
	object_->SetTranslate(transform_.translate);
}
void PlayerChargeBullet::Update(Vector3 cmrvel) {
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
	transform_.translate += velocity_+cmrvel;

	object_->SetTranslate(transform_.translate);
	object_->Update();

	if (lifeTime_ >= maxLifeTime_) {
		isActive_ = false;
	} else {
		lifeTime_++;
	}
}
void PlayerChargeBullet::Draw3D() {
	if (isActive_) {
		object_->Draw();
	}
}
void PlayerChargeBullet::Draw2D() {};
