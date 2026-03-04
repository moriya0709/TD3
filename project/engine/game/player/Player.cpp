#include "Player.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"

void Player::Initialize(Camera* camera) {
	// カメラの生成
	camera_ = camera;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};
	transform_.translate = {0.0f, 0.0f, 5.0f};

	// モデルの生成
	object_ = std::make_unique<Object>();
	object_->Initialize(camera_);
	object_->SetModel("player.obj");
	object_->SetScale(transform_.scale);
	object_->SetRotate(transform_.rotate);
	object_->SetTranslate(transform_.translate);

	statas_.hp = 100;
	statas_.attack = 10;
	statas_.speed = 0.1f;
}

void Player::Update() {
	// 入力取得
	auto input = Input::GetInstance();

	// カメラ更新
	camera_->Update();

// 1. 目標となる移動方向の初期化
	Vector3 targetDirection = {0.0f, 0.0f, 0.0f};

	// 2. 入力から「どっちに動きたいか」を決定
	if (input->PushKey(DIK_W)) {
		targetDirection.y += 1.0f;
	}
	if (input->PushKey(DIK_S)) {
		targetDirection.y -= 1.0f;
	}
	if (input->PushKey(DIK_A)) {
		targetDirection.x -= 1.0f;
	}
	if (input->PushKey(DIK_D)) {
		targetDirection.x += 1.0f;
	}

	// 3. 正規化して「目標の速度ベクトル」を作る
	Vector3 targetVelocity = {0.0f, 0.0f, 0.0f};
	float length = std::sqrt(targetDirection.x * targetDirection.x + targetDirection.y * targetDirection.y);

	if (length > 0.0f) {
		targetVelocity.x = (targetDirection.x / length) * statas_.speed;
		targetVelocity.y = (targetDirection.y / length) * statas_.speed;
	}

	// 4. 【重要】慣性の計算（現在の速度を目標速度に近づける）
	// 0.0f 〜 1.0f の値で、小さいほど滑らか（慣性が強い）になります
	float lerpFactor = 0.1f;
	

	velocity_.x = velocity_.x + (targetVelocity.x - velocity_.x) * lerpFactor;
	velocity_.y = velocity_.y + (targetVelocity.y - velocity_.y) * lerpFactor;
	velocity_.z = velocity_.z + (targetVelocity.z - velocity_.z) * lerpFactor;

	// 5. 最終的な速度を座標に加算
	transform_.translate.x += velocity_.x;
	transform_.translate.y += velocity_.y;
	transform_.translate.z += velocity_.z;
	if (transform_.translate.x < -3.5f) {
		transform_.translate.x = -3.5f; // 左端の制限
	}
	if (transform_.translate.x > 3.5f) {
		transform_.translate.x = 3.5f; // 右端の制限
	}
	if (transform_.translate.y < -0.6f) {
		transform_.translate.y = -0.6f; // 下端の制限
	}
	if (transform_.translate.y > 3.0f) {
		transform_.translate.y = 3.0f; // 上端の制限
	}

#pragma region ImGui
	ImGui::Begin("Player");                                                      // Playerウィンドウ開始
	ImGui::SliderInt("HP", &statas_.hp, 0, 100);                                 // 体力
	ImGui::SliderInt("Attack", &statas_.attack, 0, 100);                         // 攻撃力
	ImGui::SliderFloat("Speed", &statas_.speed, 0.0f, 1.0f);                     // 速度
	ImGui::SliderFloat("Homing Accuracy", &statas_.hommingAccuracy, 0.0f, 1.0f); // ホーミング精度
	ImGui::SliderFloat("Bullet Speed", &statas_.bulletSpeed, 0.0f, 1.0f);        // 弾速
	ImGui::SliderFloat("Charge Time", &statas_.chargeTime, 0.0f, 5.0f);          // チャージ時間
	ImGui::SliderFloat3("Translate", &transform_.translate.x, -3.5f, 5.0f);      // 座標
	ImGui::End();                                                                // Playerウィンドウ終了

#pragma endregion

	// 4. 変更した座標をオブジェクトにセット
	object_->SetTranslate(transform_.translate);

	// 5. オブジェクトの行列などを更新
	object_->Update();

#pragma region ライティング
	// *ライティング* //

	// 平行光
	object_->SetDirectionalLight(isDirectionalLight);
	object_->SetDirectionalLightDirection(DirectionalLightDirection);
	object_->SetDirectionalLightColor(DirectionalLightColor);
	object_->SetDirectionalLightIntensity(DirectionalLightIntensity);
	// 環境光
	object_->SetAmbientLight(isAmbientLight);
	object_->SetAmbientLightColor(AmbientLightColor);
	object_->SetAmbientLightIntensity(AmbientLightIntensity);
	// ポイントライト
	object_->SetPointLight(isPointLight);
	object_->SetPointLightColor(PointLightColor);
	object_->SetPointLightPosition(PointLightPosition);
	object_->SetPointLightIntensity(PointLightIntensity);
	// スポットライト
	object_->SetSpotLight(isSpotLight);
	object_->SetSpotLightColor(SpotLightColor);
	object_->SetSpotLightPosition(SpotLightPosition);
	object_->SetSpotLightDirection(SpotLightDirection);
	object_->SetSpotLightRange(SpotLightRange);
	object_->SetSpotLightIntensity(SpotLightIntensity);

#pragma endregion
}

void Player::Draw2D() {}

void Player::Draw3D() {
	// 3Dオブジェクト描画
	object_->Draw();
}
