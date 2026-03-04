#include "Player.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"

void Player::Initialize(Camera* camera) {
	// カメラの生成
	camera_ = camera;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};
	transform_.translate = {0.0f, 0.0f, 0.0f};

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
	// * 3Dオブジェクト* //
	if (input->PushKey(DIK_W)) {
		transform_.translate.y += statas_.speed;
	} else if (input->PushKey(DIK_S)) {
		transform_.translate.y -= statas_.speed;
	}
	if (input->PushKey(DIK_A)) {
		transform_.translate.x -= statas_.speed;
	} else if (input->PushKey(DIK_D)) {
		transform_.translate.x += statas_.speed;
	}
	object_->SetTranslate(transform_.translate);

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
