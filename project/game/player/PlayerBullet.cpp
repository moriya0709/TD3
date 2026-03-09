#include "PlayerBullet.h"
#include "player.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"

#include <cmath> // sqrt用

// Initializeに必要な引数を追加しています（レティクルの位置、最大距離、寿命）
void PlayerBullet::Initialize(const Vector3& position, Camera* camera, const Vector2 reticlePosition, const float renge) {
	// --- 1. 基本設定 ---
	transform_.scale = {0.2f, 0.2f, 0.2f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};
	transform_.translate = position; // プレイヤー（発射位置）

	object_ = std::make_unique<Object>();
	object_->Initialize(camera);
	object_->SetModel("plane.obj");

	camera_ = camera;
	lifeTime_ = 0;
	isActive_ = true;
	renge_ = renge;

	// --- 2. 正確なターゲット地点（最終地点）の計算 ---

	// 画面中心からのレティクルのズレ（ピクセル）
	float diffX = reticlePosition.x - (WindowAPI::kClientWidth / 2.0f);
	float diffY = (WindowAPI::kClientHeight / 2.0f) - reticlePosition.y;

	// カメラの垂直方向視野角（FOV）を 45度と仮定した場合の焦点距離を計算
	// ※お使いのカメラ設定のFOV（ラジアン）に合わせて調整してください
	float fovY = 0.785f; // 約45度
	float focalLength = (WindowAPI::kClientHeight / 2.0f) / std::tan(fovY / 2.0f);

	// カメラ空間での目標地点。Z軸の移動距離を renge_ とした時の X, Y を求める
	// これにより「レティクルが指す奥行き renge_ の地点」が求まる
	Vector3 targetPosCS = {(diffX / focalLength) * renge_, (diffY / focalLength) * renge_, renge_};

	// --- 3. カメラ空間からワールド空間への変換 ---

	// カメラのワールド行列（位置と回転を含む）
	Matrix4x4 camWorldMat = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, camera_->GetRotate(), camera_->GetTranslate());

	// カメラから見た目標地点を、ワールド座標の絶対位置に変換
	Vector3 targetPosWorld = VectorTransform(targetPosCS, camWorldMat);

	// --- 4. 速度の決定 ---

	// 弾の開始位置（プレイヤー）から、計算したターゲット地点（ワールド座標）へ
	// 弾がプレイヤーの横から中心に向かって「収束」するように飛ぶようになります
	velocity_.x = (targetPosWorld.x - position.x) / (float)maxLifeTime_;
	velocity_.y = (targetPosWorld.y - position.y) / (float)maxLifeTime_;
	velocity_.z = (targetPosWorld.z - position.z) / (float)maxLifeTime_;

	// 初期設定を反映
	object_->SetScale(transform_.scale);
	object_->SetRotate(transform_.rotate);
	object_->SetTranslate(transform_.translate);
}
void PlayerBullet::Update() {
	// 1. 座標の更新 (現在の座標に速度を加算)
	transform_.translate.x += velocity_.x;
	transform_.translate.y += velocity_.y;
	transform_.translate.z += velocity_.z;

	// 2. モデルへの反映
	object_->SetTranslate(transform_.translate);
	object_->Update();

	// 3. 寿命のチェック
	if (lifeTime_ >= maxLifeTime_) {
		isActive_ = false;
	} else {
		lifeTime_++;
	}
}

void PlayerBullet::Draw3D() {
	if (isActive_) {
		object_->Draw();
	}
};