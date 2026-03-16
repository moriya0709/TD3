#include "Player.h"
#include "../enemy/Enemy.h"
#include "ObjectCommon.h"
#include "PlayerChargeBullet.h"
#include "PlayerNormalBullet.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "engine/base/WindowAPI.h"
#include <algorithm>
#include <cmath>
#include <list>

// ベクトルの回転用関数
Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
	Vector3 result;
	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
	return result;
}

void Player::Initialize(Camera* camera, Style style) {
	camera_ = camera;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};

	// 1. 相対位置の初期化 (カメラの正面 10.0f)
	relativePos_ = {0.0f, 0.0f, 10.0f};

	// 初期状態のワールド座標を計算
	Vector3 camPos = camera_->GetTranslate();
	Matrix4x4 camRotMat = MakeRotateMatrix(camera_->GetRotate());
	Vector3 rotatedOffset = TransformNormal(relativePos_, camRotMat);

	transform_.translate.x = camPos.x + rotatedOffset.x;
	transform_.translate.y = camPos.y + rotatedOffset.y;
	transform_.translate.z = camPos.z + rotatedOffset.z;

	// Spriteの生成 (照準)
	reticle_ = std::make_unique<Sprite>();
	reticle_->Initialize("Resource/reticle.png");
	reticlePosition_ = {640.0f, 360.0f};
	reticle_->SetPosition(reticlePosition_);

	// モデルの生成
	playerObject_ = std::make_unique<Object>();
	playerObject_->Initialize(camera_);
	playerObject_->SetModel("player.obj");
	playerObject_->SetTranslate(transform_.translate);

	switch (style) {
	case Player::normal:
		break;
	case Player::speed:
		break;
	case Player::power:
		break;
	case Player::sniper:
		break;
	default:
		break;
	}

	// ステータス初期化
	statas_.hp = 100;
	statas_.attack = 20;
	statas_.speed = 0.2f; // XY移動は少し速い方が気持ちいいです
	statas_.haste = 10;
	statas_.chargeTime = 60;
	statas_.hommingAccuracy = 0.01f;
	statas_.renge = 80.0f;

	velocity_ = {0.0f, 0.0f, 0.0f};
	coolTime = 0;
	chargeTimer = 0;
	isCharging = false;
	ishit = false;
	damageTimer = 0;
}

void Player::Update(const std::list<std::shared_ptr<Enemy>>& enemies) {
	auto input = Input::GetInstance();

	camera_->Update();
	Vector3 camPos = camera_->GetTranslate();
	Matrix4x4 camRotMat = MakeRotateMatrix(camera_->GetRotate());

#pragma region 移動処理 (動的画面内制限)

	// 1. 入力からローカル移動方向(XY)を決定
	Vector3 moveInput = {0.0f, 0.0f, 0.0f};
	if (input->PushKey(DIK_W)) {
		moveInput.y += 1.0f;
	}
	if (input->PushKey(DIK_S)) {
		moveInput.y -= 1.0f;
	}
	if (input->PushKey(DIK_A)) {
		moveInput.x -= 1.0f;
	}
	if (input->PushKey(DIK_D)) {
		moveInput.x += 1.0f;
	}
	movePad.x = (input->GetPadLeftAxisX(0));
	if (movePad.x != 0) {
		moveInput.x += float(input->GetPadLeftAxisX(0) / 32768.0); // ゲームパッドの入力を-1.0f～1.0fに正規化
	}
	movePad.y = (input->GetPadLeftAxisY(0));

	if (input->GetPadLeftAxisY(0) != 0) {
		moveInput.y -= float(input->GetPadLeftAxisY(0) / 32768.0); // ゲームパッドの入力を-1.0f～1.0fに正規化
	}

	// 2. 正規化
	float length = std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y);
	Vector3 targetVelocity = {0.0f, 0.0f, 0.0f};
	if (length > 0.0f) {
		targetVelocity.x = (moveInput.x / length) * statas_.speed;
		targetVelocity.y = (moveInput.y / length) * statas_.speed;
	}

	// 3. 慣性適用
	float lerpFactor = 0.12f;
	velocity_.x += (targetVelocity.x - velocity_.x) * lerpFactor;
	velocity_.y += (targetVelocity.y - velocity_.y) * lerpFactor;

	// 4. 相対座標の更新
	relativePos_.x += velocity_.x;
	relativePos_.y += velocity_.y;

	// 5. 【重要】カメラの視界（Frustum）に合わせた制限の計算
	// 一般的な画角(45度)とアスペクト比(16:9)の場合
	float fovY = 0.45f; // 垂直画角（ラジアン） ※約25.7度相当。プロジェクトのカメラ設定に合わせて調整してください
	float aspect = (float)WindowAPI::kClientWidth / (float)WindowAPI::kClientHeight;

	// 距離(relativePos_.z)に応じて、画面内に収まる限界値を計算
	// 高さ = 距離 * tan(画角/2)
	float verticalLimit = relativePos_.z * std::tan(fovY / 2.0f);
	float horizontalLimit = verticalLimit * aspect;

	// 自機が少し画面内に残るようにマージンを引く
	float margin = 0.8f;
	float finalLimitX = horizontalLimit - margin;
	float finalLimitY = verticalLimit - margin;

	relativePos_.x = std::clamp(relativePos_.x, -finalLimitX, finalLimitX);
	relativePos_.y = std::clamp(relativePos_.y, -finalLimitY, finalLimitY);

	// 6. ワールド座標に変換
	Vector3 rotatedOffset = TransformNormal(relativePos_, camRotMat);
	transform_.translate.x = camPos.x + rotatedOffset.x;
	transform_.translate.y = camPos.y + rotatedOffset.y;
	transform_.translate.z = camPos.z + rotatedOffset.z;

	// 7. 見た目の回転
	float speedSq = velocity_.x * velocity_.x + velocity_.y * velocity_.y;
	if (speedSq > 0.001f) {
		transform_.rotate.z = -velocity_.x * 2.0f;
		transform_.rotate.x = velocity_.y * 1.0f;
	}
	transform_.rotate.y = camera_->GetRotate().y;

#pragma endregion

#pragma region 各種更新
	playerObject_->SetTranslate(transform_.translate);
	playerObject_->SetRotate(transform_.rotate);
	playerObject_->SetScale(transform_.scale);
	playerObject_->Update();

	if (ishit) {
		damageTimer--;
		if (damageTimer <= 0) {
			ishit = false;
		}
	}
	if (mouseMove.x == input->GetMouseScreen().x && mouseMove.y == input->GetMouseScreen().y) {
		reticlePad.x = (input->GetPadRightAxisX(0)/32768.0f);
		reticlePad.y = (input->GetPadRightAxisY(0) / 32768.0f);
		reticlePosition_.x += reticlePad.x * reticleSpeed;
		reticlePosition_.y -= reticlePad.y * reticleSpeed; // Y軸は上下逆なので減算
	} else {
		mouseMove = input->GetMouseScreen();
		reticlePosition_.x = std::clamp(mouseMove.x, 0.0f, float(WindowAPI::kClientWidth));
		reticlePosition_.y = std::clamp(mouseMove.y, 0.0f, float(WindowAPI::kClientHeight));
	}
	reticle_->SetPosition(reticlePosition_);
	reticle_->Update();

	Attack(enemies);
	UpdateBullets();
#pragma endregion

#pragma region ImGui
	ImGui::Begin("Player Config");
	ImGui::Text("Z-Distance from Camera: 5.0 (Fixed)");
	ImGui::DragFloat3("World Pos", &transform_.translate.x, 0.1f);
	ImGui::DragFloat2("Relative Pos", &relativePos_.x, 0.1f);
	ImGui::DragInt("HP", &statas_.hp, 0.1f);
	ImGui::DragInt("Attack", &statas_.attack, 0.1f);
	ImGui::DragFloat("Speed", &statas_.speed, 0.01f);
	ImGui::DragFloat("Homing Accuracy", &statas_.hommingAccuracy, 0.0001f, 0.0f, 1.0f, "%.4f");
	ImGui::DragFloat("Reticle Speed", &reticleSpeed, 0.1f);
	ImGui::DragFloat2("Move Pad", &movePad.x, 0.0f);

	ImGui::DragFloat2("Reticle Pad", &reticlePad.x, 0.0f);

	ImGui::End();
#pragma endregion
}

void Player::Draw2D() { reticle_->Draw(); }

void Player::Draw3D() {
	for (const auto& bullet : bullets) {
		bullet->Draw3D();
	}
	playerObject_->Draw();
}

Player::~Player() {
	for (auto& bullet : bullets) {
		// unique_ptrなのでdelete不要
	}
	bullets.clear();
}

void Player::Attack(const std::list<std::shared_ptr<Enemy>>& enemies) {
	auto input = Input::GetInstance();
	if (coolTime > 0) {
		coolTime--;
		return;
	}
	if (statas_.chargeTime < chargeTimer) {
		isCharging = true;
	} else {
		isCharging = false;
		chargeTimer++;
	}

	if (input->IsMouseButtonPressed(0) || input->IsPadButtonPressed(0, 5)) {
		if (isCharging) {
			// チャージ攻撃
			std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerChargeBullet>();
			newBullet->Initialize(transform_.translate, camera_, reticlePosition_, statas_.renge * 1.5f, enemies);
			newBullet->SetStatus(statas_.hommingAccuracy + 0.2f);
			bullets.push_back(std::move(newBullet)); // 修正: std::moveでunique_ptrをlistに追加
			chargeTimer = 0;                         // チャージタイマーリセット
			coolTime = statas_.haste * 2;            // チャージ攻撃後のクールタイムも長くする
		} else {
			std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerNormalBullet>();
			newBullet->Initialize(transform_.translate, camera_, reticlePosition_, statas_.renge, enemies);
			newBullet->SetStatus(statas_.hommingAccuracy);
			bullets.push_back(std::move(newBullet)); // 修正: std::moveでunique_ptrをlistに追加
			coolTime = statas_.haste;
		}
	}
}

void Player::UpdateBullets() {
	for (auto it = bullets.begin(); it != bullets.end();) {
		if (!(*it)->IsActive()) {
			it = bullets.erase(it);
		} else {
			(*it)->Update();
			++it;
		}
	}
}