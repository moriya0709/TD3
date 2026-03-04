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

	// Spriteの生成
	reticle_ = std::make_unique<Sprite>();
	reticle_->Initialize("Resource/reticle.png");
	reticlePosition_ = {0.0f, 0.0f};
	reticle_->SetPosition(reticlePosition_);

	// モデルの生成
	playerObject_ = std::make_unique<Object>();
	playerObject_->Initialize(camera_);
	playerObject_->SetModel("player.obj");
	playerObject_->SetScale(transform_.scale);
	playerObject_->SetRotate(transform_.rotate);
	playerObject_->SetTranslate(transform_.translate);

	statas_.hp = 100;
	statas_.attack = 10;
	statas_.speed = 0.1f;
}

void Player::Update() {
	// 入力取得
	auto input = Input::GetInstance();

	// カメラ更新
	camera_->Update();

#pragma region 移動処理
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

#pragma endregion

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

#pragma region オブジェクトの更新

	// 4. 変更した座標をオブジェクトにセット
	playerObject_->SetTranslate(transform_.translate);

	// 5. オブジェクトの行列などを更新
	playerObject_->Update();
#pragma endregion

#pragma region 照準の更新

	// 6. 照準の座標をマウス位置に合わせる
	// Vector2 mousePosition = Input::GetInstance()->GetMousePosition();
	// reticlePosition_ = mousePosition;
	Vector2 move{};
	if (input->PushKey(DIK_UP)) {
		move.y -= 1.0f;
	}else if (input->PushKey(DIK_DOWN)) {
		move.y += 1.0f;
	}
	if (input->PushKey(DIK_LEFT)) {
		move.x -= 1.0f;
	}
	else if (input->PushKey(DIK_RIGHT)) {
		move.x += 1.0f;
	}
	reticlePosition_.x += move.x * 20.0f; // 照準の移動速度
	reticlePosition_.y += move.y * 20.0f;
	if (reticlePosition_.x < 0.0f) {
		reticlePosition_.x = 0.0f; // 左端の制限
	}
	if (reticlePosition_.x > 1280.0f-reticle_->GetTextureSize().x) {
		reticlePosition_.x = 1280.0f - reticle_->GetTextureSize().x; // 右端の制限
	}
	if (reticlePosition_.y < 0.0f) {
		reticlePosition_.y = 0.0f; // 上端の制限
	}
	if (reticlePosition_.y > 720.0f - reticle_->GetTextureSize().y) {
		reticlePosition_.y = 720.0f - reticle_->GetTextureSize().y; // 下端の制限
	}

	reticle_->SetPosition(reticlePosition_);
	reticle_->Update();

#pragma region ライティング
	// *ライティング* //

	// 平行光
	playerObject_->SetDirectionalLight(isDirectionalLight);
	playerObject_->SetDirectionalLightDirection(DirectionalLightDirection);
	playerObject_->SetDirectionalLightColor(DirectionalLightColor);
	playerObject_->SetDirectionalLightIntensity(DirectionalLightIntensity);
	// 環境光
	playerObject_->SetAmbientLight(isAmbientLight);
	playerObject_->SetAmbientLightColor(AmbientLightColor);
	playerObject_->SetAmbientLightIntensity(AmbientLightIntensity);
	// ポイントライト
	playerObject_->SetPointLight(isPointLight);
	playerObject_->SetPointLightColor(PointLightColor);
	playerObject_->SetPointLightPosition(PointLightPosition);
	playerObject_->SetPointLightIntensity(PointLightIntensity);
	// スポットライト
	playerObject_->SetSpotLight(isSpotLight);
	playerObject_->SetSpotLightColor(SpotLightColor);
	playerObject_->SetSpotLightPosition(SpotLightPosition);
	playerObject_->SetSpotLightDirection(SpotLightDirection);
	playerObject_->SetSpotLightRange(SpotLightRange);
	playerObject_->SetSpotLightIntensity(SpotLightIntensity);

#pragma endregion
}

void Player::Draw2D() {
	// sprite描画
	reticle_->Draw(); 
}

void Player::Draw3D() {
	// 3Dオブジェクト描画
	playerObject_->Draw();
}
