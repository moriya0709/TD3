#include "Player.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "engine/base/WindowAPI.h"
#include <algorithm>
#include <cmath>
Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
	Vector3 result;

	// 行列の「回転・スケーリング」成分のみを適用する
	// (第4成分 w = 0 として扱うことで、平行移動を無視する)
	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];

	return result;
}
void Player::Initialize(Camera* camera) {
	// カメラのセット
	camera_ = camera;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};
	transform_.translate = {0.0f, 0.0f, 5.0f}; // 奥行き(Z)を固定

	// Spriteの生成 (照準)
	reticle_ = std::make_unique<Sprite>();
	reticle_->Initialize("Resource/reticle.png");
	reticlePosition_ = {640.0f, 360.0f};
	reticle_->SetPosition(reticlePosition_);

	// モデルの生成
	playerObject_ = std::make_unique<Object>();
	playerObject_->Initialize(camera_);
	playerObject_->SetModel("player.obj");

	// ステータス初期化
	statas_.hp = 100;
	statas_.attack = 10;
	statas_.speed = 0.15f;
	statas_.haste = 10;
	statas_.chargeTime = 1.0f;
	statas_.hommingAccuracy = 0.0f;
	statas_.renge = 80.0f;

	velocity_ = {0.0f, 0.0f, 0.0f};
	coolTime = 0;
}

void Player::Update() {
	auto input = Input::GetInstance();

	// カメラ更新
	camera_->Update();

#pragma region 移動処理 (XY平面基準)

	// 1. 入力からローカル移動方向を決定 (XY平面)
	Vector3 moveDirection = {0.0f, 0.0f, 0.0f};
	if (input->PushKey(DIK_W))
		moveDirection.y += 1.0f; // 上
	if (input->PushKey(DIK_S))
		moveDirection.y -= 1.0f; // 下
	if (input->PushKey(DIK_A))
		moveDirection.x -= 1.0f; // 左
	if (input->PushKey(DIK_D))
		moveDirection.x += 1.0f; // 右

	// 2. 移動ベクトルの正規化
	float length = std::sqrt(moveDirection.x * moveDirection.x + moveDirection.y * moveDirection.y);
	Vector3 worldDirection = {0.0f, 0.0f, 0.0f};

	if (length > 0.0f) {
		moveDirection.x /= length;
		moveDirection.y /= length;

		// 3. カメラの回転を考慮してワールド方向に変換
		// XY平面の移動を維持するため、回転行列を適用
		Matrix4x4 camRotMat = MakeRotateMatrix(camera_->GetRotate());
		worldDirection = TransformNormal(moveDirection, camRotMat);
	}

	// 4. 慣性(Lerp)を用いた速度計算
	Vector3 targetVelocity = {worldDirection.x * statas_.speed, worldDirection.y * statas_.speed, worldDirection.z * statas_.speed};

	float lerpFactor = 0.12f;
	velocity_.x += (targetVelocity.x - velocity_.x) * lerpFactor;
	velocity_.y += (targetVelocity.y - velocity_.y) * lerpFactor;
	velocity_.z += (targetVelocity.z - velocity_.z) * lerpFactor;

	// 5. 座標更新
	transform_.translate.x += velocity_.x;
	transform_.translate.y += velocity_.y;
	transform_.translate.z += velocity_.z;

	// 6. 進行方向を向く (XY平面上の回転)
	// XY移動に合わせて、モデルが少し傾くような演出にすると自然です
	float speedSq = velocity_.x * velocity_.x + velocity_.y * velocity_.y;
	if (speedSq > 0.001f) {
		// Z軸回転（左右への傾き）や Y軸回転を調整
		transform_.rotate.z = -velocity_.x * 2.0f; // 左右移動で少し傾く
		transform_.rotate.x = velocity_.y * 1.0f;  // 上下移動で少し傾く
	}

	// 7. 画面内制限 (カメラのXY位置を基準にした相対制限)
	Vector3 camPos = camera_->GetTranslate();
	float horizontalLimit = 6.0f;
	float verticalLimit = 4.0f;

	transform_.translate.x = std::clamp(transform_.translate.x, camPos.x - horizontalLimit, camPos.x + horizontalLimit);
	transform_.translate.y = std::clamp(transform_.translate.y, camPos.y - verticalLimit, camPos.y + verticalLimit);

#pragma endregion

#pragma region ImGui
	ImGui::Begin("Player Control");
	ImGui::Text("Move: WASD (XY Plane)");
	ImGui::SliderFloat("Speed", &statas_.speed, 0.0f, 1.0f);
	ImGui::DragFloat3("Position", &transform_.translate.x, 0.1f);
	ImGui::End();
#pragma endregion

#pragma region 各種更新
	playerObject_->SetTranslate(transform_.translate);
	playerObject_->SetRotate(transform_.rotate);
	playerObject_->SetScale(transform_.scale);
	playerObject_->Update();

	// 照準(Reticle)の更新
	Vector2 mouseMove = input->GetMouseScreen();
	reticlePosition_.x = std::clamp(mouseMove.x, 0.0f, float(WindowAPI::kClientWidth));
	reticlePosition_.y = std::clamp(mouseMove.y, 0.0f, float(WindowAPI::kClientHeight));
	reticle_->SetPosition(reticlePosition_);
	reticle_->Update();

	Attack();
	for (PlayerBullet* bullet : bullets) {
		bullet->Update();
	}
#pragma endregion
}

void Player::Draw2D() { reticle_->Draw(); }

void Player::Draw3D() {
	for (PlayerBullet* bullet : bullets) {
		bullet->Draw3D();
	}
	playerObject_->Draw();
}

Player::~Player() {
	for (PlayerBullet* bullet : bullets) {
		delete bullet;
	}
	bullets.clear();
}

void Player::Attack() {
	auto input = Input::GetInstance();
	if (coolTime > 0) {
		coolTime--;
		return;
	}
	if (input->IsMouseButtonPressed(0) || input->PushKey(DIK_SPACE)) {
		PlayerBullet* newBullet = new PlayerBullet();
		newBullet->Initialize(transform_.translate, camera_);
		newBullet->SetStatus(statas_.renge, statas_.hommingAccuracy, reticlePosition_);
		bullets.push_back(newBullet);
		coolTime = statas_.haste;
	}
}